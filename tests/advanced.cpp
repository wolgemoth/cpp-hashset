#include "../Hashset.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <thread>

/**
 * @file advanced.cpp
 * @brief Advanced tests for the functionality of the hashset.
 */
int main([[maybe_unused]] int _argc, [[maybe_unused]] char* _argv[]) {
	
	LouiEriksson::Hashset<int> hashset;

	std::cout << "~ ADVANCED TESTS ~\n";
	
	// Test 1: Large volume of data
	{
		std::cout << "Test 1: Large volume of data..." << std::flush;
		
		static constexpr int iterations = 2000000;
		
		for (int i = 0; i < iterations; ++i) {
			hashset.Add(i);
		}
		assert(hashset.size() == iterations);
		
		std::cout << "Done.\n";
	}
	
	// Test 2: Duplicate keys
	{
		std::cout << "Test 2: Duplicate keys..." << std::flush;
		
		const auto controlSize = hashset.size();
		
		for (int i = 0; i < controlSize; ++i) {
			hashset.Add(i);

			assert((hashset.Get(i).value() == i)   && "Erroneous value.");
			assert((hashset.size() == controlSize) && "Erroneous insertion.");
		}
		
		std::cout << "Done.\n";
	}
	
	// Test 3: Key collision
	{
		std::cout << "Test 3: Key collision..." << std::flush;
		
		static constexpr int iterations = 2000000;
		
		for (int i = 1; i < iterations; ++i) {
			hashset.Add(-i);
			
			assert(hashset.Get(-i).value() == -i);
		}
		
		std::cout << "Done.\n";
	}
	
	// Test 4: High churn
	{
		std::cout << "Test 4: High churn..." << std::flush;
		
		hashset.Clear();
		
		static constexpr int iterations = 2000000;
		
		for (int i = iterations; i < iterations * 2; ++i) {
			hashset.Add(i);
			hashset.Remove(i - iterations);
		}
		assert(hashset.size() == iterations);
		
		std::cout << "Done.\n";
	}
	
	// Test 5: Concurrency
	{
		std::cout << "Test 5: Concurrency..." << std::flush;
		
		hashset.Clear();
		
		static constexpr int iterations = 2000000;
		
		std::exception_ptr writerException = nullptr;
		std::exception_ptr readerException = nullptr;
		
		std::thread writer([&hashset, &writerException]() {
			
			try {
				for (int i = 0; i < iterations; ++i) {
					hashset.Add(i);
				}
			}
			catch (...) {
				writerException = std::current_exception();
			}
		});
		
		std::thread reader([&hashset, &readerException]() {
			
			try {
				int last_size = 0;
				
				while (last_size < iterations) {
					if (hashset.size() > last_size) {
						for (int i = last_size; i < hashset.size(); ++i) {
							assert((hashset.Get(i).value() == i));
						}
						last_size = hashset.size();
					}
				}
			}
			catch (...) {
				readerException = std::current_exception();
			}
		});
		
		writer.join();
		reader.join();
		
		if (writerException) { std::rethrow_exception(writerException); }
		if (readerException) { std::rethrow_exception(readerException); }

		assert(hashset.size() == iterations);
		
		std::cout << "Done.\n";
	}
	
	std::cout << "All tests passed!" << std::endl;
	
	return 0;
}