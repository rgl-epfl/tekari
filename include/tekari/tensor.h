#pragma once

#include <tekari/common.h>
#include <ostream>
#include <unordered_map>

TEKARI_NAMESPACE_BEGIN

class Tensor {
public:
	// Data type of the tensor's fields
    enum EType {
        /* Invalid/unspecified */
        EInvalid = 0,

        /* Signed and unsigned integer values */
        EUInt8,  EInt8,
        EUInt16, EInt16,
        EUInt32, EInt32,
        EUInt64, EInt64,

        /* Floating point values */
        EFloat16, EFloat32, EFloat64,
    };

    struct Field {
		// Data type of the tensor's fields
    	EType dtype;
    	// offset in the file (unused)
    	size_t offset;
        /// Specifies both rank and size along each dimension
        std::vector<size_t> shape;
        /// Const pointer to the start of the tensor
        const void* data;
    };

    /// Map the specified file into memory
    Tensor(const std::string &filename);
    
    /// Destructor
    ~Tensor();

    /// Does the file contain a field of the specified name?
    bool has_field(const std::string &name) const;

    /// Return a data structure with information about the specified field
    const Field &field(const std::string &name) const;

    /// Return a human-readable summary
    std::string to_string() const;

    /// Return the total size of the tensor's data
    size_t size() const { return m_size; }

    /// Return the name of the file from which the tensor was loaded (for compaptibility with Mitsuba's TensorFile class)
    std::string filename() const { return m_filename; }

private:
    std::unordered_map<std::string, Field> m_fields;
    std::string m_filename;
    size_t m_size;
};

extern std::ostream &operator<<(std::ostream &os, Tensor::EType value);
extern size_t type_size(Tensor::EType value);


TEKARI_NAMESPACE_END