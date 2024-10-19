#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t last_index = first_index + data.size();
  if (is_last_substring)
    end_idx = last_index;

  if (first_index <= cur_idx && cur_idx < last_index) {
    push_(first_index, std::move(data));
  } else if (cur_idx < first_index && cap_()) {
    store_(first_index, std::move(data));
  }

  if (cur_idx == end_idx)
    output_.writer().close();
}

uint64_t Reassembler::bytes_pending() const
{
  return bytes_pending_;
}

uint64_t Reassembler::cap_() const {
  return output_.writer().available_capacity();
}

void Reassembler::push_(uint64_t f_idx, string data) {
  if (f_idx <= cur_idx)
    data = data.substr(cur_idx - f_idx);
  push__(std::move(data));

  while (!storage.empty() && storage.front().ed <= cur_idx) {
    bytes_pending_ -= storage.front().str.size();
    storage.pop_front();
  }

  while (cap_() && !storage.empty()) {
    auto& seg = storage.front();
    if (seg.st <= cur_idx && cur_idx < seg.ed) {
      bytes_pending_ -= seg.str.size();
      push__(seg.str.substr(cur_idx - seg.st));
      storage.pop_front();
    } else {
      break;
    }
  }
}

void Reassembler::push__(string data) {
  if (cap_() < data.size())
    data = data.substr(0, cap_());
  cur_idx += data.size();
  output_.writer().push(data);
}

void Reassembler::store_(uint64_t f_idx, string data) {
  uint64_t boundary = cur_idx + cap_();
  uint64_t l_idx = f_idx + data.size();
  if (l_idx > boundary) {
    data = data.substr(0, boundary - f_idx);
    l_idx = f_idx + data.size();
  }
  if (data.empty()) return;

  seg _seg = {f_idx, l_idx, data};
  auto it = lower_bound(storage.begin(), storage.end(), _seg);
  if (it != storage.begin() && (it - 1)->ed > f_idx) {
    if ((it - 1)->ed >= l_idx) return;
    _seg.str = _seg.str.substr((it - 1)->ed - f_idx);
    _seg.st = (it - 1)->ed;
  }

  while (it != storage.end() && _seg.ed > it->st) {
    if (it->ed > _seg.ed) {
      _seg.str += it->str.substr(_seg.ed - it->st);
      _seg.ed = it->ed;
    }
    bytes_pending_ -= it->str.size();
    storage.erase(it);
    it = lower_bound(storage.begin(), storage.end(), _seg);
  }

  bytes_pending_ += _seg.str.size();
  storage.insert(it, _seg);
}
