#include "stream_reassembler.hh"


// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
    _output(capacity), _capacity(capacity), max_boundary_(SIZE_MAX), 
    cur_max_boundary_(capacity), cur_start_(0), unassembled_buffer_inside_() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t end_val = index+data.size()-1;

    if(eof == true){
        max_boundary_ = end_val+1;
        if(cur_start_ == max_boundary_){
            _output.end_input();
        }
    }

    if(index+data.size()>=1 && end_val<cur_start_){
        return;
    }

    string new_data_str = data;

    if(index+data.size()>=1 && end_val>=cur_max_boundary_){
        new_data_str = data.substr(0, cur_max_boundary_-index);
    }

    end_val = index+new_data_str.size()-1;

    if(new_data_str.size()!=0){
        if(index<cur_start_){
            interval new_data{cur_start_, end_val, new_data_str.substr(cur_start_-index)};
            unassembled_buffer_inside_.insert(new_data);
        }
        else{
            interval new_data{index, end_val, new_data_str};
            unassembled_buffer_inside_.insert(new_data);
        }
    }

    while(!unassembled_buffer_inside_.empty() && 
        unassembled_buffer_inside_.first_idx()==cur_start_){
        size_t first_seg_size = unassembled_buffer_inside_.first_seg().size();
        size_t written_size = _output.write(unassembled_buffer_inside_.first_seg());

        cur_start_ = unassembled_buffer_inside_.first_end()+1-(first_seg_size-written_size);
        cur_max_boundary_ = cur_start_ + _capacity;
        unassembled_buffer_inside_.pop_first();
    }

    if(cur_start_ == max_boundary_){
        _output.end_input();
    }

}

size_t StreamReassembler::unassembled_bytes() const { return unassembled_buffer_inside_.total_bytes_; }

bool StreamReassembler::empty() const { return unassembled_buffer_inside_.empty(); }