#include <tekari/tensor.h>

#include <tekari/stop_watch.h>
#include <cstdio>

TEKARI_NAMESPACE_BEGIN

/// Map the specified file into memory
Tensor::Tensor(const std::string &filename)
: m_filename(filename)
{
    // Helpfull macros to limit error-handling code duplication
#define ASSERT(cond, msg)                              \
    do {                                               \
        if (!(cond)) {                                 \
            for(auto field: m_fields)                  \
                free((void*)field.second.data);                      \
            m_fields.clear();                          \
            fclose(file);                              \
            throw std::runtime_error("Tensor: " msg);  \
        }                                              \
    } while(0)

#define SAFE_READ(vars, size, count) \
    ASSERT(fread(vars, size, count, file) == (count), "Unable to read " #vars ".")


    FILE *file = fopen(filename.c_str(), "rb");
    if (file == NULL)
        throw std::runtime_error("Unable to open file " + filename);

    ASSERT(!fseek(file, 0, SEEK_END),"Unable to seek to end of file.");

    long size = ftell(file);
    ASSERT(size != -1, "Unable to tell file cursor position.");
    m_size = static_cast<size_t>(size);
    rewind(file);

    ASSERT(m_size >= 12 + 2 + 4, "Invalid tensor file: too small, truncated?");

    uint8_t header[12], version[2];
    uint32_t n_fields;
    SAFE_READ(header, sizeof(*header), 12);
    SAFE_READ(version, sizeof(*version), 2);
    SAFE_READ(&n_fields, sizeof(n_fields), 1);

    ASSERT(memcmp(header, "tensor_file", 12) == 0, "Invalid tensor file: invalid header.");
    ASSERT(version[0] == 1 && version[1] == 0, "Invalid tensor file: unknown file version.");

    for (uint32_t i = 0; i < n_fields; ++i) {
        uint8_t dtype;
        uint16_t name_length, ndim;
        uint64_t offset;

        SAFE_READ(&name_length, sizeof(name_length), 1);
        std::string name(name_length, '\0');
        SAFE_READ((char*)name.data(), 1, name_length);
        SAFE_READ(&ndim, sizeof(ndim), 1);
        SAFE_READ(&dtype, sizeof(dtype), 1);
        SAFE_READ(&offset, sizeof(offset), 1);
        ASSERT(dtype != EInvalid && dtype <= EFloat64, "Invalid tensor file: unknown type.");

        std::vector<size_t> shape(ndim);
        size_t total_size = type_size((EType)dtype);       // no need to check here, line 43 already removes invalid types
        for (size_t j = 0; j < (size_t) ndim; ++j) {
            uint64_t size_value;
            SAFE_READ(&size_value, sizeof(size_value), 1);
            shape[j] = (size_t) size_value;
            total_size *= shape[j];
        }

        void* data = malloc(total_size);
        ASSERT(data != nullptr, "Unable to allocate enough memory.");

        long cur_pos = ftell(file);
        ASSERT(cur_pos != -1, "Unable to tell current cursor position.");
        ASSERT(fseek(file, offset, SEEK_SET) != -1, "Unable to seek to tensor offset.");
        SAFE_READ(data, 1, total_size);
        ASSERT(fseek(file, cur_pos, SEEK_SET) != -1, "Unable to seek back to current position");

        m_fields[name] =
            Field{ (EType) dtype, static_cast<size_t>(offset), shape, data };
    }
    
    fclose(file);

#undef SAFE_READ
#undef ASSERT
}

/// Does the file contain a field of the specified name?
bool Tensor::has_field(const std::string &name) const
{
    return m_fields.find(name) != m_fields.end();
}

/// Return a data structure with information about the specified field
const Tensor::Field &Tensor::field(const std::string &name) const
{
    auto it = m_fields.find(name);
    if (it == m_fields.end())
        throw std::runtime_error("Tensor: Unable to find field " + name);
    return it->second;
}

/// Return a human-readable summary
std::string Tensor::to_string() const
{
    std::ostringstream oss;
    oss << "Tensor[" << std::endl
        << "  filename = \"" << m_filename << "\"," << std::endl
        << "  size = " << size() << "," << std::endl
        << "  fields = {" << std::endl;

    size_t ctr = 0;
    for (auto it : m_fields) {
        oss << "    \"" << it.first << "\"" << " => [" << std::endl
            << "      dtype = " << it.second.dtype << "," << std::endl
            << "      offset = " << it.second.offset << "," << std::endl
            << "      shape = [";
        const auto& shape = it.second.shape;
        for (size_t j = 0; j < shape.size(); ++j) {
            oss << shape[j];
            if (j + 1 < shape.size())
                oss << ", ";
        }

        oss << "]" << std::endl;

        oss << "    ]";
        if (++ctr < m_fields.size())
            oss << ",";
        oss << std::endl;

    }

    oss << "  }" << std::endl
        << "]";

    return oss.str();
}

/// Destructor
Tensor::~Tensor()
{
    for(auto& field: m_fields)
        free((void*)field.second.data);
}

extern std::ostream &operator<<(std::ostream &os, Tensor::EType value)
{
    switch(value)
    {
        case Tensor::EInvalid:  os << "invalid"; break;
        case Tensor::EUInt8 :   os << "uint8_t"; break;
        case Tensor::EInt8:     os << "int8_t"; break;
        case Tensor::EUInt16:   os << "uint16_t"; break;
        case Tensor::EInt16:    os << "int16_t"; break;
        case Tensor::EUInt32:   os << "uint32_t"; break;
        case Tensor::EInt32:    os << "int8_t"; break;
        case Tensor::EUInt64:   os << "uint64_t"; break;
        case Tensor::EInt64:    os << "int64_t"; break;
        case Tensor::EFloat16:  os << "float16_t"; break;
        case Tensor::EFloat32:  os << "float32_t"; break;
        case Tensor::EFloat64:  os << "float64_t"; break;
        default:                os << "unkown"; break;
    }
    return os;
}

extern size_t type_size(Tensor::EType value)
{
    switch(value)
    {
        case Tensor::EInvalid:  return 0; break;
        case Tensor::EUInt8 :   return 1; break;
        case Tensor::EInt8:     return 1; break;
        case Tensor::EUInt16:   return 2; break;
        case Tensor::EInt16:    return 2; break;
        case Tensor::EUInt32:   return 4; break;
        case Tensor::EInt32:    return 4; break;
        case Tensor::EUInt64:   return 8; break;
        case Tensor::EInt64:    return 8; break;
        case Tensor::EFloat16:  return 2; break;
        case Tensor::EFloat32:  return 4; break;
        case Tensor::EFloat64:  return 8; break;
        default:                return 0; break;
    }
}

TEKARI_NAMESPACE_END