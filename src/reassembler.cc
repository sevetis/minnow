#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t last_index = first_index + data.size();
  if (is_last_substring)
    end_index_ = last_index;
  uint64_t acc_left = max(first_index, next_left_);
  uint64_t acc_right = min(
    next_left_ + output_.writer().available_capacity(),
    last_index
  );
  if (acc_right < acc_left) return;
  data = data.substr(acc_left - first_index, acc_right - acc_left);
  store_(acc_left, acc_right, std::move(data));
  push_();
}

void Reassembler::store_(uint64_t left, uint64_t right, string data) {
  auto it = pendings_.upper_bound(left);
  if (it != pendings_.begin()) { // merge left
    auto [l, d] = *std::prev(it);
    uint64_t r = l + d.size();
    if (left <= r) {
      if (right <= r)
        data = d;
      else
        data = d.substr(0, left - l) + data;
      left = l;
      pendings_.erase(l);
    }
  }
  while (it != pendings_.end() && right >= it->first) { // merge right
    auto [l, d] = *it;
    uint64_t r = l + d.size();
    if (right < r) {
      right = r;
      data = data.substr(0, l - left) + d;
    }
    ++it;
    pendings_.erase(l);
  }
  pendings_[left] = data;
}

void Reassembler::push_() {
  while (!pendings_.empty() && pendings_.begin()->first <= next_left_) {
    auto [st_index, data] = *pendings_.begin();
    pendings_.erase(st_index);
    if (next_left_ < st_index + data.size()) {
      output_.writer().push(data.substr(next_left_ - st_index));
      next_left_ = st_index + data.size();
    }
  }
  if (next_left_ == end_index_) {
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t size = 0;
  for (auto& [_, data]: pendings_)
    size += data.size();
  return size;
}
