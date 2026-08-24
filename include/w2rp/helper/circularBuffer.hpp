#ifndef W2RP_HELPER_STABLECIRCULARBUFFER_HPP
#define W2RP_HELPER_STABLECIRCULARBUFFER_HPP


#include <vector>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#include <utility> 
#include <type_traits>

template <typename T>
class stableCircularBuffer 
{
    // Ensure that T satisfies the requirements imposed by this implementation:
    // - We pre-allocate elements with `buffer_.resize(capacity_)` which requires
    //   T to be DefaultConstructible.
    // - We assign into preallocated slots using `operator=` in `push_back`, so
    //   T must be MoveAssignable or CopyAssignable.
    static_assert(std::is_default_constructible<T>::value,
                  "stableCircularBuffer<T>: T must be DefaultConstructible because the buffer pre-allocates elements with resize().");

    static_assert(std::is_move_assignable<T>::value || std::is_copy_assignable<T>::value,
                  "stableCircularBuffer<T>: T must be MoveAssignable or CopyAssignable because push_back assigns into preallocated slots.\n"
                  "If T is not assignable, consider providing an emplace-style API that constructs in-place (placement-new) instead.");

private:
    std::vector<T> buffer_;
    
    // Maps Key (SequenceNumber) -> Absolute Index (physical slot)
    std::unordered_map<size_t, size_t> lookupMap_; 
    
    // Maps Absolute Index (physical slot) -> Key (SequenceNumber)
    // This inverse map is necessary to find the key of the element being popped/overwritten, 
    // so it can be removed from the primary lookupMap_.
    std::unordered_map<size_t, size_t> reverseMap_; 
    
    size_t capacity_;
    size_t headIndex_ = 0; // Next insertion point
    size_t tailIndex_ = 0; // Oldest valid element (start of logical queue)
    size_t currentSize_ = 0;

public:
    /**     
     * @brief Construct a new stable Circular Buffer object
     * 
     * @param capacity maximum number of elements in the buffer
     */
    stableCircularBuffer(size_t capacity) : capacity_(capacity) {
        if (capacity == 0) throw std::invalid_argument("Capacity must be greater than zero.");
        
        // Allocate memory once for stability.
        buffer_.reserve(capacity_);
        buffer_.resize(capacity_);
    }

    /* ---------------------------- Basic Queue Ops ----------------------------- */
    /**
     * @brief Returns the current number of elements in the buffer.
     * 
     * @return size_t 
     */    
    size_t size() const { return currentSize_; }

    /**
     * @brief Returns the maximum capacity of the buffer.
     * 
     * @return size_t 
     */
    size_t capacity() const { return capacity_; }

    /**
     * @brief Checks if the buffer is empty.
     * 
     * @return true 
     * @return false 
     */
    bool empty() const { return currentSize_ == 0; }
    
    /**
     * @brief Inserts a new element into the buffer. If the buffer is full, it overwrites the oldest element.
     * 
     * @param key 
     * @param newElement 
     */
    template <typename U>
    void push_back(size_t key, U&& newElement) {
        if (currentSize_ == capacity_) {
            // Overwrite case: Treat as pop_front before insertion
            pop_front();
        }

        const size_t stableIndex = headIndex_;
        buffer_[stableIndex] = std::forward<U>(newElement);
        lookupMap_[key] = stableIndex; 
        reverseMap_[stableIndex] = key;

        headIndex_ = (headIndex_ + 1) % capacity_;
        currentSize_++;
    }

    /**
     * @brief Removes the oldest element from the buffer.
     * 
     */
    void pop_front() {
        if (currentSize_ == 0) {
            throw std::out_of_range("Buffer is empty.");
        }
        
        const size_t keyToRemove = reverseMap_[tailIndex_];

        lookupMap_.erase(keyToRemove);
        reverseMap_.erase(tailIndex_); // Remove the index mapping

        tailIndex_ = (tailIndex_ + 1) % capacity_;
        currentSize_--;
    }
    
    /**
     * @brief Clears the buffer and resets all internal state.
     * 
     */
    void clear() {
        lookupMap_.clear();
        reverseMap_.clear();

        headIndex_ = 0;
        tailIndex_ = 0;
        currentSize_ = 0;
        
        // Note: The underlying buffer_ (std::vector) retains its capacity, 
    }

    /* --------------------------------- Lookup --------------------------------- */
    /**
     * @brief Accesses an element using its stable Key (Sequence Number ID).
     * 
     * @param key 
     * @return T& 
     */
    T& lookup(size_t key) 
    {
        auto it = lookupMap_.find(key);
        
        if (it == lookupMap_.end()) {
            throw std::out_of_range("Key not found (stale or expired).");
        }

        const size_t absoluteIndex = it->second;
        
        return buffer_[absoluteIndex];
    }
    
    /**
     * @brief Const version of lookup to access an element using its stable Key (Sequence Number ID).
     * 
     * @param key 
     * @return const T& 
     */
    const T& lookup(size_t key) const {
        auto it = lookupMap_.find(key);
        if (it == lookupMap_.end()) {
            throw std::out_of_range("Key not found (stale or expired).");
        }
        return buffer_[it->second];
    }

    /**
     * @brief Check whether a key is present in the buffer.
     *
     * @param key the sequence number / key to check
     * @return true if the key is present
     * @return false otherwise
     */
    bool contains(size_t key) const {
        return lookupMap_.find(key) != lookupMap_.end();
    }

    /* -------------------------- Other Queue Accessors ------------------------- */
    /**
     * @brief Returns a reference to the last element inserted in the buffer.
     * 
     * @return T& 
     */
    T& back() {
        if (currentSize_ == 0) {
            throw std::out_of_range("Buffer is empty.");
        }
        // Calculate the index of the last element inserted: (headIndex_ - 1) wrapped
        size_t backIndex = (headIndex_ == 0) ? capacity_ - 1 : headIndex_ - 1;
        return buffer_[backIndex];
    }

    /**
     * @brief Returns a reference to the oldest element in the buffer.
     * 
     * @return T& 
     */
    T& front() {
        if (currentSize_ == 0) {
            throw std::out_of_range("Buffer is empty.");
        }
        return buffer_[tailIndex_];
    }   
};

#endif // W2RP_HELPER_STABLECIRCULARBUFFER_HPP