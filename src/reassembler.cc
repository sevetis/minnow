#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
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
  // Your code here.
  return bytes_pending_;
}

uint64_t Reassembler::cap_() const {
  return output_.writer().available_capacity();
}

void Reassembler::push_(uint64_t f_idx, string data) {
  if (f_idx <= cur_idx)
    data = data.substr(cur_idx - f_idx);
  push__(std::move(data));

  while (!stored.empty() && stored.front().ed <= cur_idx) {
    bytes_pending_ -= stored.front().str.size();
    stored.pop_front();
  }

  while (cap_() && !stored.empty()) {
    auto& seg = stored.front();
    if (seg.st <= cur_idx && cur_idx < seg.ed) {
      bytes_pending_ -= seg.str.size();
      push__(seg.str.substr(cur_idx - seg.st));
      stored.pop_front();
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

  for (uint64_t i = 0; i < stored.size(); i++) {
    auto& seg = stored[i];
    if (seg.ed < f_idx) continue;
    if (seg.st <= f_idx && l_idx <= seg.ed) return;
    if (l_idx < seg.st) {
      stored.insert(stored.begin() + i, {f_idx, l_idx, data});
      bytes_pending_ += data.size();
      return;
    }

    if (seg.st <= f_idx && f_idx <= seg.ed) {
      data = data.substr(seg.ed - f_idx);
      bytes_pending_ += data.size();
      seg.str += data;
      seg.ed = l_idx;
    } else if (seg.st <= l_idx && l_idx <= seg.ed) {
      data = data.substr(0, seg.st - f_idx);
      bytes_pending_ += data.size();
      seg.str = data + seg.str;
      seg.st = f_idx;
    } else if (f_idx < seg.st && seg.ed < l_idx) {
      bytes_pending_ += data.size() - seg.str.size();
      seg.str = data;
      seg.st = f_idx;
      seg.ed = l_idx;
    }

    for (uint64_t j = i + 1; j < stored.size(); j++) {
      auto& nx = stored[j];
      if (seg.ed < nx.st) break;
      if (seg.ed >= nx.ed) {
        bytes_pending_ -= nx.str.size();
        continue;
      }
      string merge = nx.str.substr(seg.ed - nx.st);
      bytes_pending_ -= nx.str.size() - merge.size();
      seg.str += merge;
      seg.ed = nx.ed;
    }
    return;
  }

  stored.push_back({f_idx, l_idx, data});
  bytes_pending_ += data.size();
}

// void Reassembler::store_(uint64_t f_idx, string data) {
//   uint64_t boundary = cur_idx + cap_();
//   uint64_t l_idx = f_idx + data.size();
//   if (l_idx > boundary) {
//     data = data.substr(0, boundary - f_idx);
//     l_idx = f_idx + data.size();
//   }
//   if (data.empty()) return;

//   uint64_t l = 0, r = stored.size() - 1;
//   while (l < r) {
//     uint64_t m = (l + r + 1) / 2;
//     if (stored[m].ed < f_idx)
//       l = m;
//     else
//       r = m - 1;
//     cout << "1";
//   }

//   if (r == stored.size() - 1) {
//     stored.push_back({f_idx, l_idx, data});
//     bytes_pending_ += data.size();
//   } else {
//     auto& seg = stored[r + 1];
//     if (l_idx < seg.st) {
//       stored.insert(stored.begin() + r + 1, {f_idx, l_idx, data});
//       bytes_pending_ += data.size();
//     } else {
//       f_idx = min(f_idx, seg.st), l_idx = max(l_idx, seg.ed);
//       bytes_pending_ += l_idx - f_idx - seg.str.size();
//       if (f_idx < seg.st && seg.ed < l_idx) {
//         seg.st = f_idx, seg.ed = l_idx, seg.str = data;
//       } else if (f_idx < seg.st) {
//         seg.str = data + seg.str.substr(l_idx - seg.st);
//         seg.st = f_idx;
//       } else {
//         seg.str = seg.str.substr(0, f_idx - seg.st) + data;
//         seg.ed = l_idx;
//       }
//     }
//   }
// }
