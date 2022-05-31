#ifndef SPONGE_LIBSPONGE_BYTE_STREAM_HH
#define SPONGE_LIBSPONGE_BYTE_STREAM_HH

#include <iostream>
#include <string>
#include <vector>

//! \brief An in-order byte stream.

//! Bytes are written on the "input" side and read from the "output"
//! side.  The byte stream is finite: the writer can end the input,
//! and then no more bytes can be written.
class ByteStream {
  private:
    // Your code here -- add private members as necessary.

    // Hint: This doesn't need to be a sophisticated data structure at
    // all, but if any of your tests are taking longer than a second,
    // that's a sign that you probably want to keep exploring
    // different approaches.

    typedef struct charCircleBuffer_ {
        size_t capacity_;
        std::string container_;
        size_t start_ = 0, end_ = 0;
        size_t cur_len_ = 0;

        charCircleBuffer_(const size_t n = 1) : capacity_(n), container_() { container_.resize(n + 1); }

        size_t avail_len() const { return capacity_ - cur_len_; }

        size_t write(const std::string &data) {
            if (data.size() == 0) {
                return 0;
            }

            size_t written_char_num = std::min(capacity_ - cur_len_, data.size());

            if (end_ + written_char_num >= capacity_ + 1) {
                size_t first_half = capacity_ + 1 - end_;
                copy(data.begin(), data.begin() + first_half, container_.begin() + end_);
                copy(data.begin() + first_half, data.begin() + written_char_num, container_.begin());
                end_ = written_char_num - first_half;
            } else {
                copy(data.begin(), data.begin() + written_char_num, container_.begin() + end_);
                end_ = end_ + written_char_num;
            }

            cur_len_ += written_char_num;

            return written_char_num;
        }

        size_t remaining_capacity() const { return capacity_ - cur_len_; }

        std::string peek_output(const size_t len) const {
            if (buffer_empty()) {
                return "";
            }

            size_t rt_len = std::min(cur_len_, len);

            if (start_ + rt_len >= capacity_ + 1) {
                auto remain_half = start_ + rt_len - capacity_ - 1;
                std::string first_half(container_.begin() + start_, container_.begin() + capacity_ + 1);
                std::string second_half(container_.begin(), container_.begin() + remain_half);
                return first_half + second_half;
            } else {
                std::string rt(container_.begin() + start_, container_.begin() + start_ + rt_len);
                return rt;
            }
        }

        size_t pop_output(const size_t len) {
            if (buffer_empty()) {
                return 0;
            }

            size_t pop_char_num = std::min(cur_len_, len);

            if (start_ + pop_char_num >= capacity_ + 1) {
                start_ = pop_char_num - (capacity_ + 1 - start_);
            } else {
                start_ += len;
            }

            cur_len_ -= pop_char_num;

            return pop_char_num;
        }

        std::string read(const size_t len) {
            std::string rt = peek_output(len);
            pop_output(len);
            return rt;
        }

        size_t buffer_size() const { return cur_len_; }

        bool buffer_empty() const { return cur_len_ == 0; }

        void resize(size_t n) {
            start_ = 0;
            end_ = 0;
            cur_len_ = 0;
            capacity_ = n;
            container_.resize(n);
        }

    } charCircleBuffer;

    charCircleBuffer container_;
    size_t written_bytes_ = 0;
    size_t read_bytes_ = 0;
    bool input_ended_ = false;

    bool _error{};  //!< Flag indicating that the stream suffered an error.

  public:
    //! Construct a stream with room for `capacity` bytes.
    ByteStream(const size_t capacity);

    //! \name "Input" interface for the writer
    //!@{

    //! Write a string of bytes into the stream. Write as many
    //! as will fit, and return how many were written.
    //! \returns the number of bytes accepted into the stream
    size_t write(const std::string &data);

    //! \returns the number of additional bytes that the stream has space for
    size_t remaining_capacity() const;

    //! Signal that the byte stream has reached its ending
    void end_input();

    //! Indicate that the stream suffered an error.
    void set_error() { _error = true; }
    //!@}

    //! \name "Output" interface for the reader
    //!@{

    //! Peek at next "len" bytes of the stream
    //! \returns a string
    std::string peek_output(const size_t len) const;

    //! Remove bytes from the buffer
    void pop_output(const size_t len);

    //! Read (i.e., copy and then pop) the next "len" bytes of the stream
    //! \returns a string
    std::string read(const size_t len);

    //! \returns `true` if the stream input has ended
    bool input_ended() const;

    //! \returns `true` if the stream has suffered an error
    bool error() const { return _error; }

    //! \returns the maximum amount that can currently be read from the stream
    size_t buffer_size() const;

    //! \returns `true` if the buffer is empty
    bool buffer_empty() const;

    //! \returns `true` if the output has reached the ending
    bool eof() const;
    //!@}

    //! \name General accounting
    //!@{

    //! Total number of bytes written
    size_t bytes_written() const;

    //! Total number of bytes popped
    size_t bytes_read() const;
    //!@}

    void set_error(bool error) { _error = error; }

    size_t avail_len() const { return container_.avail_len(); }
};

#endif  // SPONGE_LIBSPONGE_BYTE_STREAM_HH
