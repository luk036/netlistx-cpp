/**
 * @file gen.hpp
 * @brief Minimal C++20 coroutine Generator for lazy sequence generation
 */

#pragma once

#include <coroutine>
#include <exception>
#include <iterator>
#include <utility>

namespace py {

    /**
     * @brief Minimal C++20 coroutine generator (Python-style)
     *
     * Lazily yields values of type T using co_yield.
     * Models a Python generator with iterable interface.
     *
     * @tparam T Value type to yield
     */
    template <typename T> class Generator {
      public:
        struct promise_type {
            T current_value;

            auto yield_value(T value) noexcept {
                current_value = std::move(value);
                return std::suspend_always{};
            }

            auto initial_suspend() noexcept { return std::suspend_always{}; }
            auto final_suspend() noexcept { return std::suspend_always{}; }
            void return_void() noexcept {}
            void unhandled_exception() noexcept { std::terminate(); }

            auto get_return_object() noexcept {
                return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
            }
        };

      private:
        using handle_type = std::coroutine_handle<promise_type>;
        handle_type coro_{};

        explicit Generator(handle_type coro) noexcept : coro_(coro) {}

      public:
        Generator() noexcept = default;
        ~Generator() {
            if (coro_) coro_.destroy();
        }

        Generator(Generator&& other) noexcept : coro_(std::exchange(other.coro_, {})) {}
        Generator& operator=(Generator&& other) noexcept {
            if (this != &other) {
                if (coro_) coro_.destroy();
                coro_ = std::exchange(other.coro_, {});
            }
            return *this;
        }

        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;

        class iterator {
            handle_type coro_{};

          public:
            using iterator_category = std::input_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;

            iterator() noexcept = default;

            /**
             * @brief Construct an iterator from a coroutine handle
             * @param[in] coro Coroutine handle to wrap
             */
            explicit iterator(handle_type coro) noexcept : coro_(coro) {}

            /**
             * @brief Advance to the next yielded value
             * @return iterator& Reference to this iterator
             */
            iterator& operator++() {
                coro_.resume();
                if (coro_.done()) coro_ = {};
                return *this;
            }
            void operator++(int) { ++*this; }

            /**
             * @brief Dereference to get the current yielded value
             * @return const T& Current value
             */
            const T& operator*() const noexcept { return coro_.promise().current_value; }
            T& operator*() noexcept { return coro_.promise().current_value; }

            bool operator==(const iterator& other) const noexcept { return coro_ == other.coro_; }
            bool operator!=(const iterator& other) const noexcept { return !(*this == other); }
        };

        /**
         * @brief Get iterator to the first yielded value
         * @return iterator Iterator to begin
         */
        iterator begin() {
            if (!coro_) return iterator{};
            coro_.resume();
            if (coro_.done()) return iterator{};
            return iterator{coro_};
        }

        /**
         * @brief Get past-the-end iterator
         * @return iterator Sentinel iterator
         */
        iterator end() noexcept { return iterator{}; }
    };

}  // namespace py
