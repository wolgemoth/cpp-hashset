/*
 * MIT License
 *
 * Copyright (c) 2024 Louis Eriksson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef LOUIERIKSSON_HASHSET_HPP
#define LOUIERIKSSON_HASHSET_HPP

//#define HASHSET_SUPPRESS_EXCEPTION_WARNING // Uncomment if you wish to remove the warning about possible unhandled exceptions.

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <vector>

namespace LouiEriksson {
	
	/**
	 * @mainpage Version 1.0.1
	 * @details Custom Hashset implementation accepting a customisable type.
	 *          This implementation requires that the given type is compatible with std::hash and is copyable.
	 * @tparam T Entry type of the Hashset.
	 */
	template<typename T>
	class Hashset final {
		
		inline static std::shared_mutex s_Lock;

	private:
		
		/** @brief Buckets of the Hashset. */
		std::vector<std::vector<T>> m_Buckets;
		
		/** @brief Current number of elements within the Hashset. */
		size_t m_Size;
		
		/**
		 * @brief Calculate the hashcode of a given object using std::hash.
		 * @param[in] _item Item to calculate hash of.
		 * @return Hashcode of _item.
		 * @throw std::exception If the type of _item is not supported by std::hash.
		 */
		static constexpr size_t GetHashcode(const T& _item) {
			return std::hash<T>()(_item);
		}
		
		/**
		 * @brief Reinitialise the Hashset. An expensive operation that increases the Hashset's capacity.
		 *
		 * @param _newSize The new size of the Hashset.
		 */
		void Resize(const size_t& _newSize) {

			std::vector<std::vector<T>> shallow_cpy(m_Buckets);

			try {

				m_Size = 0U;
				m_Buckets.clear();
				m_Buckets.resize(_newSize > 0U ? _newSize : 1U);

				for (auto& bucket : shallow_cpy) {
					for (auto& key1 : bucket) {

						if (m_Size >= m_Buckets.size()) {
							Resize(m_Buckets.size() * 2U);
						}

						// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
						const auto hash = GetHashcode(key1);
						const auto i = hash % m_Buckets.size();

						auto& bucket = m_Buckets[i];

						auto exists = false;
						for (auto& key2 : bucket) {

							if (GetHashcode(key2) == hash) {
								exists = true;

								key2 = key1;

								break;
							}
						}

						if (!exists) {
							m_Size++;

							bucket.emplace_back(std::move(key1));
						}
					}
				}
			}
			catch (const std::exception& e){
				m_Buckets = shallow_cpy;
			}
		}

		/**
		 * @brief Retrieves a reference to the entry within the Hashset with the given key, if one exists.
		 * This method will throw an exception if no entry is found. Consider using Get() for safe access instead.
		 *
		 * @param[in] _key Key of the entry to retrieve.
		 * @return Out value result.
		 * @throw std::runtime_error If no entry is found.
		 * @see Get(const T& _key, Tv& _out)
		 */
		constexpr const T& Return(const T& _key) const {

			const T* result = nullptr;

			if (!m_Buckets.empty()) {

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				size_t hash = GetHashcode(_key);
				size_t i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				for (auto& key : bucket) {

					if (GetHashcode(key) == hash) {
						result = &key;

						break;
					}
				}
			}

			if (result == nullptr) {
				throw std::runtime_error("Attempted to access a nonexistent entry from the Hashset.");
			}

			return *result;
		}

	public:

		/**
		 * @brief Initialise Hashset.
		 * @param[in] _capacity Initial capacity of the Hashset. Must be larger than 0.
		 */
		constexpr Hashset(const size_t& _capacity = 1U) : m_Size(0U) {
			m_Buckets.resize(_capacity);
		}

		/**
		 * @brief Initialise Hashset using a collection of key-value pairs.
		 * @details Please note: The provided collection should be distinct. Otherwise, some data loss may occur as duplicate entries will be ignored.
		 *
		 * @param[in] _items A collection of key-value pairs.
		 * @param[in] _capacity Initial capacity of the Hashset. If a value less than 1 is assigned, it will use the size of the provided collection.
		 */
		constexpr Hashset(const std::initializer_list<T>& _items, const size_t& _capacity = 0U) : m_Size(0U) {

			size_t auto_capacity = _capacity;

			if (auto_capacity < 1U) {
				auto_capacity = std::max<size_t>(_items.size(), 1U);
			}

			m_Buckets.resize(auto_capacity);

			for (const auto& item : _items) {
				Assign(item);
			}
		}

		struct optional_ref final {

			friend Hashset;

		private:

			using optional_t = std::optional<std::reference_wrapper<const T>>;

			const optional_t m_Optional;

			explicit optional_ref(optional_t&& _optional) : m_Optional(_optional) {};

		public:

			[[nodiscard]] const T& value()                const { return m_Optional.value(); }
			[[nodiscard]] const T& value_or(const T&& _t) const { return m_Optional.value_or(_t); }

			[[nodiscard]] bool has_value() const { return m_Optional.has_value(); }

			[[nodiscard]] const T& operator  *() const { return  value(); }
			[[nodiscard]] const T* operator ->() const { return &value(); }

			[[nodiscard]] operator bool() const { return has_value(); }
		};

		/**
		 * @brief Returns the number of items stored within the Hashset.
		 * @return The number of items stored within the Hashset.
		 */
		[[nodiscard]] const size_t& size() const noexcept {

			const std::shared_lock lock(s_Lock);

			return m_Size;
		}

		/**
		 * @brief Is the Hashset empty?
		 * @return Returns true if the Hashset contains no entries.
		 */
		[[nodiscard]] bool empty() const noexcept {
			return size() == 0U;
		}

		/**
		 * @brief Queries for the existence of an item in the Hashset.
		 *
		 * @param[in] _key Key of the entry.
		 * @return True if successful, false otherwise.
		 */
		bool ContainsKey(const T& _key) const noexcept {

			const std::shared_lock lock(s_Lock);

			auto result = false;

			try {

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				size_t hash = GetHashcode(_key);
				size_t i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				for (auto& key : bucket) {

					if (GetHashcode(key) == hash) {
						result = true;

						break;
					}
				}
			}
			catch (...) {}

			return result;
		}

		/**
		 * @brief Queries for the existence of an item in the Hashset.
		 *
		 * @param[in] _key Key of the entry.
		 * @param[out] _exception (optional) A pointer to any exception caught during the operation.
		 * @return True if successful, false otherwise.
		 */
		bool ContainsKey(const T& _key, std::exception_ptr& _exception) const noexcept {

			const std::shared_lock lock(s_Lock);

			auto result = false;

			try {

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				size_t hash = GetHashcode(_key);
				size_t i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				for (auto& key : bucket) {

					if (GetHashcode(key) == hash) {
						result = true;

						break;
					}
				}
			}
			catch (...) {
				_exception = std::current_exception();
			}

			return result;
		}

		/**
		 * @brief Inserts a new entry into the Hashset with given key and value, if one does not already exist.
		 * If you are trying to modify an existing key, see Hashset::Assign.
		 *
		 * @param[in] _key Key of the entry.
		 * @return True if successful, false otherwise.
		 */
		bool Add(const T& _key) noexcept {

			const std::unique_lock lock(s_Lock);

			auto result = true;

			try {

				if (m_Size >= m_Buckets.size()) {
					Resize(m_Buckets.size() * 2U);
				}

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				const auto hash = GetHashcode(_key);
				const auto i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				// In the case of a hash collision, determine if the key is unique.
				// We will treat duplicate insertions as a mistake on the developer's part and return failure.
				for (auto& key : bucket) {
					if (GetHashcode(key) == hash) {
						result = false;

						break;
					}
				}

				// Insert the item into the bucket.
				if (result) {
					m_Size++;

					bucket.emplace_back(_key);
				}
			}
			catch (...) {}

			return result;
		}

		/**
		 * @brief Inserts a new entry into the Hashset with given key and value, if one does not already exist.
		 * If you are trying to modify an existing key, see Hashset::Assign.
		 *
		 * @param[in] _key Key of the entry.
		 * @param[out] _exception (optional) A pointer to any exception caught during the operation.
		 * @return True if successful, false otherwise.
		 */
		bool Add(const T& _key, std::exception_ptr& _exception) noexcept {

			const std::unique_lock lock(s_Lock);

			auto result = true;

			try {

				if (m_Size >= m_Buckets.size()) {
					Resize(m_Buckets.size() * 2U);
				}

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				const auto hash = GetHashcode(_key);
				const auto i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				// In the case of a hash collision, determine if the key is unique.
				// We will treat duplicate insertions as a mistake on the developer's part and return failure.
				for (auto& key : bucket) {
					if (GetHashcode(key) == hash) {
						result = false;

						break;
					}
				}

				// Insert the item into the bucket.
				if (result) {
					m_Size++;

					bucket.emplace_back(_key);
				}
			}
			catch (...) {
				_exception = std::current_exception();
			}

			return result;
		}

		/**
		 * @brief Inserts a new entry into the Hashset with given key and value using move semantics, if one does not already exist.
		 * If you are trying to modify an existing key, see Hashset::Assign.
		 *
		 * @param[in] _key Key of the entry.
		 * @return True if successful, false otherwise.
		 */
		bool Add(const T&& _key) noexcept {

			const std::unique_lock lock(s_Lock);

			auto result = true;

			try {

				if (m_Size >= m_Buckets.size()) {
					Resize(m_Buckets.size() * 2U);
				}

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				const auto hash = GetHashcode(_key);
				const auto i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				// In the case of a hash collision, determine if the key is unique.
				// We will treat duplicate insertions as a mistake on the developer's part and return failure.
				for (auto& key : bucket) {
					if (GetHashcode(key) == hash) {
						result = false;

						break;
					}
				}

				// Insert the item into the bucket.
				if (result) {
					m_Size++;

					bucket.emplace_back(_key);
				}
			}
			catch (...) {}

			return result;
		}

		/**
		 * @brief Inserts a new entry into the Hashset with given key and value using move semantics, if one does not already exist.
		 * If you are trying to modify an existing key, see Hashset::Assign.
		 *
		 * @param[in] _key Key of the entry.
		 * @param[out] _exception (optional) A pointer to any exception caught during the operation.
		 * @return True if successful, false otherwise.
		 */
		bool Add(const T&& _key, std::exception_ptr& _exception) noexcept {

			const std::unique_lock lock(s_Lock);

			auto result = true;

			try {

				if (m_Size >= m_Buckets.size()) {
					Resize(m_Buckets.size() * 2U);
				}

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				const auto hash = GetHashcode(_key);
				const auto i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				// In the case of a hash collision, determine if the key is unique.
				// We will treat duplicate insertions as a mistake on the developer's part and return failure.
				for (auto& key : bucket) {
					if (GetHashcode(key) == hash) {
						result = false;

						break;
					}
				}

				// Insert the item into the bucket.
				if (result) {
					m_Size++;

					bucket.emplace_back(_key);
				}
			}
			catch (...) {
				_exception = std::current_exception();
			}

			return result;
		}

		/**
		 * @brief Inserts or replaces an entry within the Hashset with the given key.
		 *
		 * @param[in] _key Key of the entry.
		 */
		void Assign(const T& _key) noexcept {

			const std::unique_lock lock(s_Lock);

			try {

				if (m_Size >= m_Buckets.size()) {
					Resize(m_Buckets.size() * 2U);
				}

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				const auto hash = GetHashcode(_key);
				const auto i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				auto exists = false;
				for (auto& key : bucket) {

					if (GetHashcode(key) == hash) {
						exists = true;

						key = _key;

						break;
					}
				}

				if (!exists) {
					m_Size++;

					bucket.emplace_back(_key);
				}
			}
			catch (...) {}
		}

		/**
		 * @brief Inserts or replaces an entry within the Hashset with the given key.
		 *
		 * @param[in] _key Key of the entry.
		 * @param[out] _exception (optional) A pointer to any exception caught during the operation.
		 */
		void Assign(const T& _key, std::exception_ptr& _exception) noexcept {

			const std::unique_lock lock(s_Lock);

			try {

				if (m_Size >= m_Buckets.size()) {
					Resize(m_Buckets.size() * 2U);
				}

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				const auto hash = GetHashcode(_key);
				const auto i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				auto exists = false;
				for (auto& key : bucket) {

					if (GetHashcode(key) == hash) {
						exists = true;

						key = _key;

						break;
					}
				}

				if (!exists) {
					m_Size++;

					bucket.emplace_back(_key);
				}
			}
			catch (...) {
				_exception = std::current_exception();
			}
		}

		/**
		 * @brief Inserts or replaces an entry within the Hashset with the given key using move semantics.
		 *
		 * @param[in] _key Key of the entry.
		 */
		void Assign(T&& _key) noexcept {

			const std::unique_lock lock(s_Lock);

			try {

				if (m_Size >= m_Buckets.size()) {
					Resize(m_Buckets.size() * 2U);
				}

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				const auto hash = GetHashcode(_key);
				const auto i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				auto exists = false;
				for (auto& key : bucket) {

					if (GetHashcode(key) == hash) {
						exists = true;

						key = _key;

						break;
					}
				}

				if (!exists) {
					m_Size++;

					bucket.emplace_back(std::move(_key));
				}
			}
			catch (...) {}
		}

		/**
		 * @brief Inserts or replaces an entry within the Hashset with the given key using move semantics.
		 *
		 * @param[in] _key Key of the entry.
		 * @param[out] _exception (optional) A pointer to any exception caught during the operation.
		 */
		void Assign(T&& _key, std::exception_ptr& _exception) noexcept {

			const std::unique_lock lock(s_Lock);

			try {

				if (m_Size >= m_Buckets.size()) {
					Resize(m_Buckets.size() * 2U);
				}

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				const auto hash = GetHashcode(_key);
				const auto i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				auto exists = false;
				for (auto& key : bucket) {

					if (GetHashcode(key) == hash) {
						exists = true;

						key = _key;

						break;
					}
				}

				if (!exists) {
					m_Size++;

					bucket.emplace_back(std::move(_key));
				}
			}
			catch (...) {
				_exception = std::current_exception();
			}
		}

		/**
		 * @brief Removes entry with given key from the Hashset.
		 *
		 * @param[in] _key Key of the entry to be removed.
		 * @return True if successful, false otherwise.
		 */
		bool Remove(const T& _key) noexcept {

			const std::unique_lock lock(s_Lock);

			bool result = false;

			try {

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				const size_t hash = GetHashcode(_key);
				const size_t i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				// In the case of accessing a "collided" hash, find the value in the bucket using equality checks.
				for (auto itr = bucket.begin(); itr < bucket.end(); itr++) {

					if (GetHashcode(*itr) == hash) {
						result = true;

						bucket.erase(itr);

						break;
					}
				}

				m_Size -= static_cast<size_t>(result);

			}
			catch (...) {}

			return result;
		}

		/**
		 * @brief Removes entry with given key from the Hashset.
		 *
		 * @param[in] _key Key of the entry to be removed.
		 * @param[out] _exception (optional) A pointer to any exception caught during the operation.
		 * @return True if successful, false otherwise.
		 */
		bool Remove(const T& _key, std::exception_ptr& _exception) noexcept {

			const std::unique_lock lock(s_Lock);

			bool result = false;

			try {

				// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
				const size_t hash = GetHashcode(_key);
				const size_t i = hash % m_Buckets.size();

				auto& bucket = m_Buckets[i];

				// In the case of accessing a "collided" hash, find the value in the bucket using equality checks.
				for (auto itr = bucket.begin(); itr < bucket.end(); itr++) {

					if (GetHashcode(*itr) == hash) {
						result = true;

						bucket.erase(itr);

						break;
					}
				}

				m_Size -= static_cast<size_t>(result);

			}
			catch (...) {
				_exception = std::current_exception();
			}

			return result;
		}

		/**
		 * @brief Retrieves the value associated with the given key from the fnv1a table.
		 *
		 * @tparam T The type of the key.
		 * @param[in] _key The key to retrieve the value for.
		 * @return An optional reference to the value associated with the key, or std::nullopt if the key is not present.
		 * @note This function is noexcept.
		 */
		optional_ref Get(const T& _key) const noexcept {

			const std::shared_lock lock(s_Lock);

			typename optional_ref::optional_t result = std::nullopt;

			try {

				if (!m_Buckets.empty()) {

					// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
					size_t hash = GetHashcode(_key);
					size_t i = hash % m_Buckets.size();

					auto& bucket = m_Buckets[i];

					for (auto& key : bucket) {

						if (GetHashcode(key) == hash) {
							result = std::cref(key);
							break;
						}
					}
				}
			}
			catch (...) {}

			return optional_ref(std::move(result));
		}

		/**
		 * @brief Retrieves the value associated with the given key from the fnv1a table.
		 *
		 * @tparam T The type of the key.
		 * @param[in] _key The key to retrieve the value for.
		 * @param[out] _exception (optional) A pointer to any exception caught during the operation.
		 * @return An optional reference to the value associated with the key, or std::nullopt if the key is not present.
		 * @note This function is noexcept.
		 */
		optional_ref Get(const T& _key, std::exception_ptr& _exception) const noexcept {

			const std::shared_lock lock(s_Lock);

			typename optional_ref::optional_t result = std::nullopt;

			try {

				if (!m_Buckets.empty()) {

					// Create an index by taking the key's hash value and "wrapping" it with the number of buckets.
					size_t hash = GetHashcode(_key);
					size_t i = hash % m_Buckets.size();

					auto& bucket = m_Buckets[i];

					for (auto& key : bucket) {

						if (GetHashcode(key) == hash) {
							result = std::cref(key);
							break;
						}
					}
				}
			}
			catch (...) {
				_exception = std::current_exception();
			}

			return optional_ref(std::move(result));
		}

		/**
		 * @brief Trims unused entries from the end of the Hashset.
		 */
		void Trim() {

			const std::unique_lock lock(s_Lock);

			size_t trimStart = 1U;

			for (size_t i = trimStart; i < m_Buckets.size(); ++i) {
				if (m_Buckets[i].size() != 0U) {
					trimStart = i + 1U;
				}
			}

			if (trimStart < m_Buckets.size()) {
				m_Buckets.erase(m_Buckets.begin() + trimStart);
			}
		}

		/**
		 * @brief Returns a shallow copy of all entries stored within the Hashset.
		 * @return A shallow copy of all entries stored within the Hashset.
		 */
		[[nodiscard]] std::vector<T> GetAll() const {

			const std::shared_lock lock(s_Lock);

			std::vector<T> result;

			for (const auto& bucket : m_Buckets) {
				for (const auto& key : bucket) {
					result.emplace_back(key);
				}
			}

			return result;
		}

		/**
		 * @brief Reserves memory for the container to have a minimum capacity of _newSize elements.
		 *
		 * @param[in] _newSize The minimum capacity to reserve for the container.
		 */
		void Reserve(const std::size_t& _newSize) {

			const std::unique_lock lock(s_Lock);

			if (m_Size < _newSize) {
				Resize(_newSize);
			}
		}

		/**
		 * @brief Clears all entries from the Hashset.
		 */
		void Clear() noexcept {

			const std::unique_lock lock(s_Lock);

			try {
				m_Buckets.clear();
				m_Size = 0U;
			}
			catch (const std::exception& e) {
				std::cerr << e.what() << std::endl;
			}
			catch (...) {}
		}

		/**
		 * @brief Retrieves a reference to the entry within the Hashset with the given key, if one exists.
		 * This method will throw an exception if no entry is found. Consider using Get() for safe access instead.
		 *
		 * @param[in] _key Key of the entry to retrieve.
		 * @return Out value result.
		 * @throw std::runtime_error If no entry is found.
		 *
		 * @see Get(const T& _key, Tv& _out)
		 */
#ifndef HASHSET_SUPPRESS_EXCEPTION_WARNING
		[[deprecated("This function does not guarantee exception-safety and will explicitly throw if no entry exists. Consider using Get() if exception-safe access is required.\nSuppress this warning by defining \"HASHSET_SUPPRESS_UNSAFE_WARNING\".")]]
#endif
		const T& operator[](const T& _key) const {

			const std::shared_lock lock(s_Lock);

		    return Return(_key);
		}
		
		/* ITERATORS */
		
		/**
		 * @class const_iterator
		 * @brief Represents an iterator to traverse through the elements in a Hashset.
		 */
		class const_iterator final {
		
			friend Hashset;
			
			using outer_itr = typename std::vector<std::vector<T>>::const_iterator;
			using inner_itr = typename std::vector<T>::const_iterator;
			
		private:
			
			outer_itr m_Outer;
			outer_itr m_Outer_End;
			inner_itr m_Inner;
			
			constexpr const_iterator(const outer_itr& _outer,
			                         const outer_itr& _outer_end,
			                         const inner_itr& _inner) :
				    m_Outer(_outer),
				m_Outer_End(_outer_end),
				    m_Inner(_inner)
			{
				if (m_Outer != m_Outer_End && m_Inner == m_Outer->end()) {
					++(*this);
				}
			}
			
		public:
			
			const const_iterator& operator ++() {
				
				if (++m_Inner == m_Outer->end()) {
					
					while (++m_Outer != m_Outer_End) {
						
						if (!m_Outer->empty()) {
							m_Inner = m_Outer->begin();
							break;
						}
					}
					if (m_Outer == m_Outer_End) {
						m_Inner = inner_itr();
					}
				}
				return *this;
			}
			
			const T& operator *() const { return *m_Inner; }
			
			bool operator ==(const const_iterator& other) const { return ((m_Outer == other.m_Outer) && (m_Outer == m_Outer_End || m_Inner == other.m_Inner)); }
			bool operator !=(const const_iterator& other) const { return !operator ==(other); }
		};
		
		constexpr const_iterator begin() const { return const_iterator(m_Buckets.begin(), m_Buckets.end(), m_Buckets.empty() ? typename std::vector<T>::const_iterator() : m_Buckets.begin()->begin()); }
		constexpr const_iterator   end() const { return const_iterator(m_Buckets.end(),   m_Buckets.end(), typename std::vector<T>::const_iterator()); }
	};
	
} // LouiEriksson

#endif //LOUIERIKSSON_HASHSET_HPP
