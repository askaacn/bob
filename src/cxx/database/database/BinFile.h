/**
 * @file src/cxx/database/database/BinFile.h
 * @author <a href="mailto:Laurent.El-Shafey@idiap.ch">Laurent El Shafey</a> 
 *
 * @brief This class can be used to load and store arrays from/to binary files.
 */

#ifndef TORCH_DATABASE_BINFILE_H
#define TORCH_DATABASE_BINFILE_H 1

#include "core/StaticComplexCast.h"
#include "database/BinFileHeader.h"
#include "database/InlinedArrayImpl.h"

namespace Torch {
  /**
   * \ingroup libdatabase_api
   * @{
   *
   */
  namespace database {

    /**
     * Defines the flags that might be used when loading/storing a file
     * containing blitz arrays.
     */
    enum _BinFileFlag {
      _append  = 1L << 0,
      _in      = 1L << 3,
      _out     = 1L << 4
    };

    /**
     * This class can be used for loading and storing multiarrays from/to
     * binary files
     */
    class BinFile
    {
      public:
        /**
         * Defines the bitmask type for providing information about the type of
         * the stream.
         */
        typedef _BinFileFlag openmode;
        static const openmode append  = _append;
        static const openmode in      = _in;
        static const openmode out     = _out; 

        /**
         * Constructor
         */
        BinFile(const std::string& filename, openmode f);

        /**
         * Destructor
         */
        ~BinFile();

        /**
         * Closes the BinFile
         */
        void close();

        /** 
         * Puts an Array of a given type into the output stream/file. If the
         * type/shape have not yet been set, it is set according to the type
         * and shape given in the blitz array, otherwise the type/shape should
         * match or an exception is thrown.
         *
         * Please note that blitz::Array<> will be implicetly constructed as
         * required and respecting those norms.
         */
        void write(const detail::InlinedArrayImpl& data);

        /**
         * Load one blitz++ multiarray from the input stream/file All the
         * multiarrays saved have the same dimensions.
         */
        template <typename T, int D> inline blitz::Array<T,D> read() {
          return read().cast<T,D>(); 
        }
 
        template <typename T, int D> inline blitz::Array<T,D> read(size_t
            index) { return read(index).cast<T,D>(); }
        
        detail::InlinedArrayImpl read(); 
        detail::InlinedArrayImpl read (size_t index);

        /**
         * Gets the Element type
         *
         * @warning An exception is thrown if nothing was written so far
         */
        inline Torch::core::array::ElementType getElementType() const { 
          headerInitialized(); 
          return m_header.m_elem_type; 
        }

        /**
         * Gets the number of dimensions
         *
         * @warning An exception is thrown if nothing was written so far
         */
        inline size_t getNDimensions() const {  
          headerInitialized(); 
          return m_header.m_n_dimensions; 
        }

        /**
         * Gets the shape of each array
         *
         * @warning An exception is thrown if nothing was written so far
         */
        inline const size_t* getShape() const { 
          headerInitialized(); 
          return m_header.m_shape; 
        }

        /**
         * Gets the shape of each array in a blitz format
         *
         * @warning An exception is thrown if nothing was written so far
         */
        template<int D> inline void getShape (blitz::TinyVector<int,D>& res) 
          const {
          headerInitialized(); 
          m_header.getShape(res);
        }

        /**
         * Gets the number of samples/arrays written so far
         *
         * @warning An exception is thrown if nothing was written so far
         */
        inline size_t getNSamples() const { 
          headerInitialized(); 
          return m_n_arrays_written; 
        }

        /**
         * Gets the number of elements per array
         *
         * @warning An exception is thrown if nothing was written so far
         */
        inline size_t getNElements() const { 
          headerInitialized(); 
          return m_header.getNElements(); 
        }

        /**
         * Gets the size along a particular dimension
         *
         * @warning An exception is thrown if nothing was written so far
         */
        inline size_t getSize(size_t dim_index) const { 
          headerInitialized(); 
          return m_header.getSize(dim_index); 
        }

        /**
         * Initializes the binary file with the given type and shape.
         */
        inline void initBinaryFile(Torch::core::array::ElementType type,
            size_t ndim, const size_t& shape[array::N_MAX_DIMENSIONS_ARRAY]) {
            initHeader(type, ndim, shape);
        }

      private: //Some stuff I need privately

        /**
         * Checks if the end of the binary file is reached
         */
        inline void endOfFile() {
          if(m_current_array >= m_header.m_n_samples ) throw IndexError();
        }

        /** 
         * Checks that the header has been initialized, and raise an
         * exception if not
         */
        inline void headerInitialized() const { 
          if (!m_header_init) throw Uninitialized();
        }

        /**
         * Initializes the header of the (output) stream with the given type
         * and shape
         */
        void initHeader(Torch::core::array::ElementType type, size_t ndim,
            const size_t* shape);

      private: //representation

        bool m_header_init;
        size_t m_current_array;
        size_t m_n_arrays_written;
        std::fstream m_stream;
        BinFileHeader m_header;
        openmode m_openmode;
    };

    inline _BinFileFlag operator&(_BinFileFlag a, _BinFileFlag b) { 
      return _BinFileFlag(static_cast<int>(a) & static_cast<int>(b)); 
    }

    inline _BinFileFlag operator|(_BinFileFlag a, _BinFileFlag b) { 
      return _BinFileFlag(static_cast<int>(a) | static_cast<int>(b)); 
    }

    inline _BinFileFlag operator^(_BinFileFlag a, _BinFileFlag b) { 
      return _BinFileFlag(static_cast<int>(a) ^ static_cast<int>(b)); 
    }

    inline _BinFileFlag& operator|=(_BinFileFlag& a, _BinFileFlag b) { 
      return a = a | b; 
    }

    inline _BinFileFlag& operator&=(_BinFileFlag& a, _BinFileFlag b) { 
      return a = a & b; 
    }

    inline _BinFileFlag& operator^=(_BinFileFlag& a, _BinFileFlag b) { 
      return a = a ^ b; 
    }

    inline _BinFileFlag operator~(_BinFileFlag a) { 
      return _BinFileFlag(~static_cast<int>(a)); 
    }

  }
  /**
   * @}
   */
}

#endif /* TORCH_DATABASE_BINFILE_H */
