#include <iostream>
#include <vector>
#include <cstring>  // Fixed: Added missing include for strcpy
#include <stdexcept> // For exception handling

class Calculator {
private:
    int* data;
    size_t size_;
    
public:
    // Fixed: Added proper destructor
    ~Calculator() {
        delete[] data;
    }
    
    // Fixed: Proper constructor with error checking and initialization
    Calculator(size_t size) : data(nullptr), size_(size) {
        if (size == 0) {
            throw std::invalid_argument("Size cannot be zero");
        }
        data = new int[size]();
        // Fixed: Initialize array elements to zero
        for (size_t i = 0; i < size_; ++i) {
            data[i] = 0;
        }
    }
    
    // Fixed: Added copy constructor (Rule of Three)
    Calculator(const Calculator& other) : data(nullptr), size_(other.size_) {
        data = new int[size_];
        for (size_t i = 0; i < size_; ++i) {
            data[i] = other.data[i];
        }
    }
    
    // Fixed: Added assignment operator (Rule of Three)
    Calculator& operator=(const Calculator& other) {
        if (this != &other) {
            delete[] data;
            size_ = other.size_;
            data = new int[size_];
            for (size_t i = 0; i < size_; ++i) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }
    
    // Fixed: Added division by zero check
    int calculate(int a, int b) {
        if (b == 0) {
            throw std::invalid_argument("Division by zero");
        }
        return a / b;
    }
    
    // Fixed: Pass by const reference for efficiency
    void processArray(const std::vector<int>& vec) {
        // Fixed: Off-by-one error - use < instead of <=
        for (size_t i = 0; i < vec.size(); ++i) {
            std::cout << vec[i] << std::endl;
        }
    }
    
    // Fixed: Memory leak eliminated
    std::string getName() const {
        return std::string("Calculator");
    }
};

int main() {
    try {
        Calculator calc(10);
        
        // Fixed: Initialize variables properly
        int x = 10, y = 5;
        int result = calc.calculate(x, y);
        std::cout << "Result: " << result << std::endl;
        
        std::vector<int> numbers = {1, 2, 3, 4, 5};
        calc.processArray(numbers);
        
        // Fixed: Proper RAII instead of raw pointers
        {
            Calculator dynCalc(5);
            std::string name = dynCalc.getName();
            std::cout << "Name: " << name << std::endl;
        } // Automatically cleaned up here
        
        // Fixed: Added return statement
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
