#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

using namespace std;

ByteStream::ByteStream(const size_t capacity): container_ () { 
    container_.resize(capacity);
}

size_t ByteStream::write(const string &data) {
    auto rt = container_.write(data);
    written_bytes_ += rt;
    return rt;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return container_.peek_output(len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    auto rt = container_.pop_output(len);
    read_bytes_ += rt;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    auto rt = container_.read(len);
    return rt;
}

void ByteStream::end_input() {
    input_ended_ = true;
}

bool ByteStream::input_ended() const { 
    return input_ended_; 
}

size_t ByteStream::buffer_size() const { 
    return container_.buffer_size(); 
}

bool ByteStream::buffer_empty() const { 
    return container_.buffer_empty();
}

bool ByteStream::eof() const { 
    return buffer_empty() && input_ended();
}

size_t ByteStream::bytes_written() const { 
    return written_bytes_; 
}

size_t ByteStream::bytes_read() const { 
    return read_bytes_; 
}

size_t ByteStream::remaining_capacity() const { 
    return container_.remaining_capacity(); 
}