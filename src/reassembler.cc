#include "reassembler.hh"
#include <cstdint>

#include <iostream>
#include <string>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t last_index = first_index + data.size();
  if (is_last_substring)
    end_index_ = last_index;
cout << first_index << endl;
  uint64_t acc_left = max(first_index, next_left_);
  uint64_t acc_right = min(
    next_left_ + output_.writer().available_capacity(),
    last_index
  );
  if (acc_right < acc_left) return;
cout << "acc left: " << acc_left << " acc right: " << acc_right << endl;
  data = data.substr(acc_left - first_index, acc_right - acc_left);
cout << " data: " << data << endl;

for (auto& [i, s]: pendings_) {
  cout << i << " -> " << s << endl;
}
cout << "------------" << endl;

  auto it = pendings_.upper_bound(acc_left);
  if (it != pendings_.begin()) {
    auto [l, d] = *std::prev(it);
    if (acc_left <= l + d.size()) {
cout << "L: " << l << endl;
cout << "<1>: " << data << endl;
      data = d.substr(0, acc_left - l);
      acc_left = l;
      pendings_.erase(l);
    }
  }
  while (it != pendings_.end() && acc_right >= it->first) {
    auto [l, d] = *it;
    uint64_t r = l + data.size();
    if (acc_right < r) {
      acc_right = l + data.size();
cout << "2" << endl;
      data = data.substr(0, l - acc_left) + d;
    }
    ++it;
    pendings_.erase(l);
  }
  pendings_[acc_left] = data;

for (auto& [i, s]: pendings_) {
  cout << i << " -> " << s << endl;
}

  push_();
}

void Reassembler::push_() {
  while (!pendings_.empty() && pendings_.begin()->first <= next_left_) {
    auto [st_index, data] = *pendings_.begin();
    pendings_.erase(st_index);
    if (next_left_ < st_index + data.size()) {
cout << "3" << endl;
cout << "nxt: " << next_left_ << " st: " << st_index << " data: " << data << endl;
      output_.writer().push(data.substr(next_left_ - st_index));
      next_left_ = st_index + data.size();
    }
  }
  if (next_left_ == end_index_) {
    output_.writer().close();
  }
cout << "+++++++++++" << endl;
for (auto& [i, s]: pendings_) {
  cout << i << " -> " << s << endl;
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
