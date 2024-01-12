#include "stream_reassembler.hh"

#include <algorithm>
#include <iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    if (eof) {
        _is_eof = true;
        if (index == 0) {
            _output.end_input();
        }
    }
    // } else if (_is_eof && empty()) {  // eof had been added into unassembled list.
    //     _output.end_input();
    // }
    if (index == _next_index) {
        // First, verify if this data segment already exists in the list. If present, remove it, then perform operations
        // based on the list's data. Importantly, in this scenario, the pair should be the list's head.
        std::string str;
        bool flag = false;
        for (auto it = _unassembled.begin(); it != _unassembled.end();) {
            if (it->first == index) {
                str = it->second;
                flag = true;
                _unassembled.erase(it);
                break;
            }
            if (it->first > index) {
                break;
            }
            ++it;
        }
				// for example for the index = 0, the segment will impossible be added into list
        if (!flag) {  
            // The data must be inserted into the list to confirm it aligns with the spatial constraints.
						_unassembled.push_front({_next_index, data}); 
            sort_overlap_space();
            auto it = _unassembled.begin();
            str = it->second;
            _unassembled.erase(it);
        }
        _output.write(str);
        _next_index += str.size();
				// This step is crucial for pushing adjacent but non-overlapping segments.
				// eg: Initialized (capacity = 2)
        // Action:      substring submitted with data "bX", index `1`, eof `0`
        // Expectation: net bytes assembled = 0
        // Action:      substring submitted with data "a", index `0`, eof `0`
        while (_unassembled.size() && _unassembled.front().first == _next_index) {
            push_substring(_unassembled.front().second, _unassembled.front().first, false);
        }
        if (_is_eof && empty()) {  // eof had been added into unassembled list.
            _output.end_input();
        }
    } else if (index < _next_index) {
        // cut the string in the pair from index up to next_index. If a substring remains, retain it; otherwise, discard
        // the pair.
        if ((index + data.size()) <= _next_index) {
            for (auto it = _unassembled.begin(); it != _unassembled.end();) {
                if (it->first == index) {
                    _unassembled.erase(it);
                    return;
                }
                ++it;
            }
            return;
        }
        std::string str = data.substr(_next_index - index);
        // If retained, insert the pair into the list, then sort the list.
        _unassembled.push_front({_next_index, str});
        sort_overlap_space();
        // Finally, invoke push_substring and insert the list's first element into parameter.
        push_substring(_unassembled.front().second, _unassembled.front().first, false);

    } else {  // index > next_index
        // For index>next_index, repeat the steps without calling push_substring, as this data insertion doesn't create
        // a continuous data stream.
        if (data.size() > 0) {
            _unassembled.push_back({index, data});
        }
        sort_overlap_space();
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    size_t res = 0;
    for (const auto &[key, val] : _unassembled) {
        res += val.size();
    }
    return res;
}

bool StreamReassembler::empty() const { return _unassembled.empty(); }

void StreamReassembler::sort_overlap() {
    _unassembled.sort([](const auto &a, const auto &b) { return a.first < b.first; });
    for (auto it = _unassembled.begin(); it != _unassembled.end(); /* no increment here */) {
        auto next_it = std::next(it);
        if (next_it != _unassembled.end()) {
            int overlap = (it->first + it->second.size()) - next_it->first;
            if (overlap > 0) {
                // 只追加不重叠的部分
                if (overlap < static_cast<int>(next_it->second.size())) {
                    it->second += next_it->second.substr(overlap);
                }  // else all of the next segment is overlap, so do nothing

                _unassembled.erase(next_it);  // important! do not update it.
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

void StreamReassembler::sort_overlap_space() {
    sort_overlap();
    size_t free_size = _capacity - _output.buffer_size();
    size_t list_size = unassembled_bytes();
    // If it surpasses the allowed capacity (capacity - out_put.buffer_size()), begin truncating from the list's
    // end until it fits.
    if (list_size > free_size) {
        size_t space_waiting_free = list_size - free_size;
        if (_is_eof) {
            _is_eof = false;
        }
        while (space_waiting_free) {
            auto &it = _unassembled.back();
            if (it.second.size() < space_waiting_free) { // do not contain equal! otherwise will lead to segment fault. When list just has one element, then is empty, but we have "sort_overlap_space() auto it = _unassembled.begin();"!
                space_waiting_free -= it.second.size();
                _unassembled.pop_back();
            } else {
                std::string tmp = it.second;
                size_t len = it.second.size();
                it.second = tmp.substr(0, len - space_waiting_free);
                space_waiting_free = 0;
            }
        }
    }
}