#pragma once

#include <memory>
#include <cstring>
#include <iterator>

namespace eng
{

    template<typename Type, typename Allocator = std::allocator<Type>>
    struct vector
    {
        using value_type = Type;
        using size_type = size_t;
        using allocator_type = Allocator;

        using iterator = value_type*;
        using const_iterator = value_type const*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    private:
        using allocator_traits = std::allocator_traits<allocator_type>;

        static constexpr double GROWTH_FACTOR = 1.5;
        static constexpr size_type DEFAULT_SIZE = 4;

    public:
        constexpr vector() noexcept:
            mAllocator(),
            mSize(0),
            mCapacity(0),
            mData(nullptr)
        { }
        constexpr vector(const allocator_type& alloc) :
            mAllocator(alloc),
            mSize(0),
            mCapacity(0),
            mData(nullptr)
        { }
        constexpr vector(const vector& other, const allocator_type& alloc) :
            mAllocator(alloc),
            mSize(other.mSize),
            mCapacity(other.mCapacity),
            mData(allocator_traits::allocate(mAllocator, mCapacity))
        {
            std::uninitialized_copy_n(other.mData, mSize, mData);
        }
        constexpr vector(const vector& other) :
            vector(other, allocator_traits::select_on_copy_construction(other.mAllocator))
        { }
        constexpr vector(vector&& other, const allocator_type& alloc) noexcept :
            vector(alloc)
        {
            swap(other);
        }
        constexpr vector(vector&& other) noexcept :
            vector(other, other.mAllocator)
        { }

        constexpr vector(size_type count, const allocator_type& alloc = allocator_type{ }) :
            mAllocator(alloc),
            mSize(count),
            mCapacity(count),
            mData(allocator_traits::allocate(mAllocator, mCapacity))
        {
            std::uninitialized_default_construct_n(mData, count);
        }
        constexpr vector(size_type count, const value_type& value, const allocator_type& alloc = allocator_type{ }) :
            mAllocator(alloc),
            mSize(count),
            mCapacity(count),
            mData(allocator_traits::allocate(mAllocator, mCapacity))
        {
            std::uninitialized_fill_n(mData, count, value);
        }

        constexpr vector(std::initializer_list<value_type> list, const allocator_type alloc = { }) :
            vector(alloc)
        {
            assign(list.begin(), list.end());
        }

        constexpr vector& operator=(const vector& other)
        {
            if (allocator_traits::propagate_on_container_copy_assignment::value && mAllocator != other.mAllocator)
            {
                vector temp{ other, other.mAllocator };
                swap_with_allocator(temp);
                return *this;
            }

            assign(other.begin(), other.end());
            return *this;
        }
        constexpr vector& operator=(vector&& other) noexcept
        {
            if (allocator_traits::propagate_on_container_move_assignment::value || mAllocator == other.mAllocator)
            {
                swap_with_allocator(other);
                return *this;
            }
            
            if constexpr (std::is_nothrow_move_constructible_v<value_type> || !std::is_copy_constructible_v<value_type>)
            {
                assign(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
            }
            else
            {
                assign(other.begin(), other.end());
            }
            return *this;
        }
        constexpr vector& operator=(std::initializer_list<value_type> list)
        {
            assign(list.begin(), list.end());
            return *this;
        }

        ~vector()
        {
            reset();
        }

        constexpr value_type& push_back(value_type value)
        {
            return emplace_back(std::move(value));
        }
        template<typename... Arguments>
        constexpr value_type& emplace_back(Arguments&&... args)
        {
            if (mSize >= mCapacity)
            {
                reallocate(mSize ? mSize * GROWTH_FACTOR : DEFAULT_SIZE);
            }

            std::construct_at(mData + mSize, std::forward<Arguments>(args)...);
            return mData[mSize++];
        }
        constexpr void pop_back()
        {
            if (!mSize)
            {
                throw std::out_of_range("eng::vector<Type, Allocator>::pop_back() was called on an empty container");
            }
            if (mSize * GROWTH_FACTOR * GROWTH_FACTOR <= mCapacity) // More than 2 reallocations away
            {
                reallocate(mSize * GROWTH_FACTOR);
            }

            allocator_traits::destroy(mAllocator, mData + --mSize);
        }

        constexpr void clear()
        {
            while (mSize)
            {
                allocator_traits::destroy(mAllocator, mData + --mSize);
            }
        }
        constexpr void reset()
        {
            clear();
            allocator_traits::deallocate(mAllocator, mData, mCapacity);
            mData = nullptr;
        }
        constexpr void reserve(size_type newCapacity)
        {
            if (mCapacity < newCapacity)
            {
                reallocate(newCapacity);
            }
        }
        constexpr void resize(size_type newSize)
        {
            size_type oldSize = mSize;
            if (mCapacity < newSize)
            {
                reallocate(newSize);
            }
            std::uninitialized_default_construct_n(mData, (oldSize < newSize ? newSize - oldSize : 0));
        }
        constexpr void shrink_to_fit()
        {
            if (mSize >= mCapacity)
            {
                reallocate(mSize);
            }
        }

        constexpr void assign(size_type count, const value_type& value)
        {
            value_type* data = mData;
            if (count > mCapacity)
            {
                data = allocator_traits::allocate(mAllocator, count);
            }

            size_t assignRange = std::min(count, mSize);

            std::fill_n(data, assignRange, value);
            std::destroy_n(data + assignRange, (assignRange < mSize ? mSize - assignRange : 0));
            std::uninitialized_fill_n(data + mSize, (mSize < count ? count - mSize : 0), value);

            if (count > mCapacity)
            {
                allocator_traits::deallocate(mAllocator, mData, mCapacity);
                mData = data;
                mCapacity = count;
            }
            mSize = count;
        }
        template<typename InputIt>
        constexpr void assign(InputIt begin, InputIt end)
        {
            value_type* data = mData;
            size_type rangeSize = end - begin;

            if (rangeSize > mCapacity)
            {
                data = allocator_traits::allocate(mAllocator, rangeSize);
            }

            if constexpr (std::is_trivially_copyable_v<value_type>)
            {
                std::memcpy(data, begin, rangeSize);
            }
            else
            {
                size_t assignRange = std::min(rangeSize, mSize);

                std::copy_n(begin, assignRange, data);
                std::destroy_n(data + assignRange, (assignRange < mSize ? mSize - assignRange : 0));
                std::uninitialized_copy_n(begin + assignRange, (assignRange < rangeSize ? rangeSize - assignRange : 0), data + mSize);
            }

            if (rangeSize > mCapacity)
            {
                allocator_traits::deallocate(mAllocator, mData, mCapacity);
                mData = data;
                mCapacity = rangeSize;
            }
            mSize = rangeSize;
        }
        constexpr void assign(std::initializer_list<value_type> list)
        {
            assign(list.begin(), list.end());
        }
        constexpr void swap(vector& other) noexcept
        {
            if constexpr (allocator_traits::propogate_on_container_swap::value)
            {
                swap_with_allocator(other);
            }
            else
            {
                swap_without_allocator(other);
            }
        }
        
        constexpr value_type& operator[](size_type index)
        {
            return mData[index];
        }
        constexpr const value_type& operator[](size_type index) const
        {
            return mData[index];
        }

        constexpr value_type& at(size_type index)
        {
            if (index >= mSize)
            {
                throw std::out_of_range("vector::at(size_type) tried to access an element out of bounds");
            }
            return mData[index];
        }
        constexpr const value_type& at(size_type index) const
        {
            if (index >= mSize)
            {
                throw std::out_of_range("vector::at(size_type) tried to access an element out of bounds");
            }
            return mData[index];
        }

        static constexpr size_type max_size()
        {
            return std::numeric_limits<size_type>::max();
        }

        constexpr bool empty() const
        {
            return !mSize;
        }
        constexpr size_type size() const
        {
            return mSize;
        }
        constexpr size_type capacity() const
        {
            return mCapacity;
        }
        constexpr value_type* data() const
        {
            return mData;
        }
        constexpr allocator_type& get_allocator()
        {
            return *this;
        }
        constexpr const allocator_type& get_allocator() const
        {
            return *this;
        }

        constexpr value_type& front()
        {
            return mData[0];
        }
        constexpr const value_type& front() const
        {
            return mData[0];
        }
        constexpr value_type& back()
        {
            return mData[mSize - 1];
        }
        constexpr const value_type& back() const
        {
            return mData[mSize - 1];
        }

        constexpr iterator begin() const
        {
            return mData;
        }
        constexpr iterator end() const
        {
            return mData + mSize;
        }

        constexpr const_iterator cbegin() const
        {
            return mData;
        }
        constexpr const_iterator cend() const
        {
            return mData + mSize;
        }

        constexpr reverse_iterator rbegin() const
        {
            return mData + mSize - 1;
        }
        constexpr reverse_iterator rend() const
        {
            return mData - 1;
        }

        constexpr const_reverse_iterator crbegin() const
        {
            return mData + mSize - 1;
        }
        constexpr const_reverse_iterator crend() const
        {
            return mData - 1;
        }

        constexpr auto operator<=>(const vector& other) const
        {            
            for (size_type i = 0; i < mSize; ++i)
            {
                if (auto cmp = mData[i] <=> other.mData[i]; cmp != 0)
                {
                    return cmp;
                }
            }
            
            return mSize <=> other.mSize;
        }
        constexpr bool operator==(const vector& other) const = default;

    protected:
        constexpr void reallocate(size_type newCapacity)
        {
            value_type* newData = allocator_traits::allocate(mAllocator, newCapacity);
            size_type newSize = std::min(newCapacity, mSize);   // Account for shrinking
            
            // Relocate data to new buffer; cannot use stdlib algs due to lack of ranged move_if_noexcept and destroy_at
            if constexpr (std::is_trivially_copyable_v<value_type>) // Try to memcpy for trivial types
            {
                std::memcpy(newData, mData, newSize);
            }
            else
            {
                for (size_t i = 0; i < newSize; ++i)
                {
                    // Move construct to new buffer if move is noexcept, otherwise copy
                    std::construct_at(newData + i, std::move_if_noexcept(mData[i]));
                    std::destroy_at(mData + i);
                }
            }
            
            // Destroy rest of old buffer
            std::destroy_n(mData, (newSize < mSize ? mSize - newSize : 0));

            // Redirect member variables to new data
            allocator_traits::deallocate(mAllocator, mData, mCapacity);
            mSize = newSize;
            mCapacity = newCapacity;
            mData = newData;
        }
        constexpr void swap_with_allocator(vector& other) noexcept
        {
            using std::swap;
            swap(mAllocator, other.mAllocator);
            swap_without_allocator(other);
        }
        constexpr void swap_without_allocator(vector& other) noexcept
        {
            using std::swap;
            swap(mSize, other.mSize);
            swap(mCapacity, other.mCapacity);
            swap(mData, other.mData);

        }

    private:
        [[no_unique_address]] allocator_type mAllocator;
        size_type mSize;
        size_type mCapacity;
        value_type* mData;
    };

}

namespace std
{    

    template<typename Type>
    constexpr void swap(eng::vector<Type>& first, eng::vector<Type>& second) noexcept
    {
        first.swap(second);
    }

}