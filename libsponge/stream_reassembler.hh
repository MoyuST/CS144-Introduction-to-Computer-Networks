#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <algorithm>
#include <cstdint>
#include <set>
#include <string>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
    size_t max_boundary_;
    size_t cur_max_boundary_;
    size_t cur_start_;

    typedef struct interval_ {
        size_t start_;
        size_t end_;
        std::string seg_;
        interval_(size_t start, size_t end, std::string seg) : start_(start), end_(end), seg_(seg) {}

        bool operator<(const interval_ &right) const { return this->start_ <= right.start_; }
    } interval;

    typedef struct unassembled_buffer_ {
        std::set<interval> buffer_;
        size_t total_bytes_ = 0;

        unassembled_buffer_() : buffer_() {}

        bool empty() const { return total_bytes_ == 0; }

        size_t first_idx() {
            if (empty()) {
                return 65535;  // minimun of the maximun value of size_t
            }

            return (*(buffer_.begin())).start_;
        }

        std::string first_seg() {
            if (empty()) {
                return "";
            }

            return (*(buffer_.begin())).seg_;
        }

        size_t first_end() {
            if (empty()) {
                return 65535;  // minimun of the maximun value of size_t
            }

            return (*(buffer_.begin())).end_;
        }

        void pop_first() {
            if (!empty()) {
                total_bytes_ -= (*(buffer_.begin())).seg_.size();
                buffer_.erase(buffer_.begin());
            }
        }

        void insert(const interval &data) {
            if (empty()) {
                total_bytes_ += data.seg_.size();
                buffer_.insert(data);
            } else {
                auto rt = std::lower_bound(
                    buffer_.begin(),
                    buffer_.end(),
                    data.start_,
                    [](const interval &entry, const size_t &left_val) -> bool { return entry.start_ < left_val; });

                if (rt != buffer_.end()) {
                    auto left_bound = (*rt).start_;
                    if (data.end_ < left_bound) {
                        auto rt_right_bound =
                            std::lower_bound(buffer_.begin(),
                                             buffer_.end(),
                                             data.start_,
                                             [](const interval &entry, const size_t &left_val) -> bool {
                                                 return entry.end_ < left_val;
                                             });
                        if (rt_right_bound != buffer_.end() && (*rt_right_bound).end_ >= data.start_ &&
                            (*rt_right_bound).end_ < (*rt).start_) {
                            if ((*rt_right_bound).end_ < data.end_) {
                                interval merge_data =
                                    interval{(*rt_right_bound).start_,
                                             data.end_,
                                             (*rt_right_bound).seg_ +
                                                 data.seg_.substr((*rt_right_bound).end_ - data.start_ + 1)};
                                total_bytes_ -= (*rt_right_bound).seg_.size();
                                buffer_.erase(rt_right_bound);
                                insert(merge_data);
                            } else {
                            }
                        } else {
                            total_bytes_ += data.seg_.size();
                            buffer_.insert(data);
                        }
                    } else if (data.end_ == left_bound) {
                        interval new_interval{
                            data.start_, (*rt).end_, data.seg_.substr(0, (*rt).start_ - data.start_) + (*rt).seg_};
                        total_bytes_ -= (*rt).seg_.size();
                        buffer_.erase(rt);
                        insert(new_interval);
                    } else {
                        if (data.end_ <= (*rt).end_) {
                            interval new_interval{
                                data.start_, (*rt).end_, data.seg_.substr(0, (*rt).start_ - data.start_) + (*rt).seg_};
                            total_bytes_ -= (*rt).seg_.size();
                            buffer_.erase(rt);
                            insert(new_interval);
                        } else {
                            total_bytes_ -= (*rt).seg_.size();
                            buffer_.erase(rt);
                            insert(data);
                        }
                    }
                } else {
                    auto rt_right_bound = std::lower_bound(
                        buffer_.begin(),
                        buffer_.end(),
                        data.start_,
                        [](const interval &entry, const size_t &left_val) -> bool { return entry.end_ < left_val; });

                    if (rt_right_bound != buffer_.end()) {
                        if ((*rt_right_bound).end_ < data.end_) {
                            interval merge_data{
                                (*rt_right_bound).start_,
                                data.end_,
                                (*rt_right_bound).seg_ +
                                    data.seg_.substr(data.seg_.size() - (data.end_ - (*rt_right_bound).end_))};
                            total_bytes_ -= (*rt_right_bound).seg_.size();
                            buffer_.erase(rt_right_bound);
                            insert(merge_data);
                        }
                    } else {
                        total_bytes_ += data.seg_.size();
                        buffer_.insert(data);
                    }
                }
            }
        }

    } unassembled_buffer;

    unassembled_buffer unassembled_buffer_inside_;

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;

    size_t first_reassembled_idx() const { return cur_start_; }
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
