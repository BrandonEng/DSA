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
        using allocator_traits = std::allocator_traits<Allocator>;

        static constexpr double GROWTH_FACTOR = 1.5;
        static constexpr size_type DEFAULT_SIZE = 4;

    public:
        constexpr vector() noexcept:
            mAllocator(),
            mSize(0),
            mCapacity(0),
            mData(nullptr)
        { }
        constexpr vector(const Allocator& alloc) :
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
            if constexpr (std::is_trivially_copyable_v<value_type>)
            {
                std::memcpy(mData, other.mData, other.mSize);
            }
            else
            {
                for (size_type i = 0; i < mSize; ++i)
                {
                    allocator_traits::construct(mAllocator, mData + i, other[i]);
                }
            }
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
            for (size_type i = 0; i < count; ++i)
            {
                allocator_traits::construct(mAllocator, mData + i);
            }
        }
        constexpr vector(size_type count, const value_type& value, const allocator_type& alloc = allocator_type{ }) :
            mAllocator(alloc),
            mSize(0),
            mCapacity(count),
            mData(allocator_traits::allocate(mAllocator, mCapacity))
        {
            assign(count, value);
        }

        constexpr vector(std::initializer_list<value_type> list, const allocator_type alloc = { }) :
            mAllocator(alloc),
            mSize(0),
            mCapacity(list.size()),
            mData(allocator_traits::allocate(mAllocator, mCapacity))
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

            if constexpr (std::is_nothrow_move_assignable_v<value_type> && std::is_nothrow_move_constructible_v<value_type>)
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

        constexpr ~vector()
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

            allocator_traits::construct(mAllocator, mData + mSize, std::forward<Arguments>(args)...);
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
            for (size_type i = oldSize; i < newSize; ++i)
            {
                allocator_traits::construct(mAllocator, mData + i);
            }
        }
        constexpr void shrink_to_fit()
        {
            if (mSize != mCapacity)
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

            for (size_type i = 0; i < assignRange; ++i)
            {
                data[i] = value;
            }
            for (size_type i = assignRange; i < mSize; ++i)
            {
                allocator_traits::destroy(mAllocator, data + i);
            }
            for (size_type i = mSize; i < count; ++i)
            {
                allocator_traits::construct(mAllocator, data + i, value);
            }

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

                for (size_type i = 0; i < assignRange; ++i)
                {
                    data[i] = *begin++;
                }
                for (size_type i = assignRange; i < mSize; ++i)
                {
                    allocator_traits::destroy(mAllocator, data + i);
                }
                for (size_type i = mSize; i < rangeSize; ++i)
                {
                    allocator_traits::construct(mAllocator, data + i, *begin++);
                }
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

        static constexpr size_type max_size() const
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

    private:
        constexpr void reallocate(size_type newCapacity)
        {
            value_type* newData = allocator_traits::allocate(mAllocator, newCapacity);
            size_type newSize = std::min(newCapacity, mSize);   // Account for shrinking
            
            // Relocate data to new buffer
            if constexpr (std::is_trivially_copyable_v<value_type>) // Try to memcpy for trivial types
            {
                std::memcpy(newData, mData, newSize);
            }
            else
            {
                for (size_t i = 0; i < newSize; ++i)
                {
                    // Move construct to new buffer if move is noexcept
                    allocator_traits::construct(mAllocator, newData + i, std::move_if_noexcept(mData[i]));
                    allocator_traits::destroy(mAllocator, mData + i);
                }
            }
            
            // Destroy rest of old buffer
            for (size_type i = newSize; i < mSize; ++i)
            {
                allocator_traits::destroy(mAllocator, mData + i);
            }

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

    template<typename Type>
    constexpr void swap(vector<Type>& first, vector<Type>& second) noexcept
    {
        first.swap(second);
    }

}