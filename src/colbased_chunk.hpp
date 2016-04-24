/*
    ParaText: parallel text reading
    Copyright (C) 2016. wise.io, Inc.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
  Coder: Damian Eads.
 */

#ifndef PARATEXT_COLBASED_CHUNK_HPP
#define PARATEXT_COLBASED_CHUNK_HPP

#include "parse_params.hpp"
#include "widening_vector.hpp"
#include <typeindex>

namespace ParaText {

  /*
    Represents a chunk of parsed column data for a col-based CSV parser.
  */
  class ColBasedChunk {
  public:
    /*
      Creates a new chunk with an empty name.
     */
    ColBasedChunk() {}

    /*
      Creates a new chunk.

      \param column_name      The name of the column for the chunk.
     */
    ColBasedChunk(const std::string &column_name)
      : column_name_(column_name) {}

    /*
     * Destroys this chunk.
     */
    virtual ~ColBasedChunk()             {}

    /*
     * Passes a floating point datum to the column handler. If categorical
     * data was previously passed to this handler, this datum will be converted
     * to a string and treated as categorical.
     */
    void process_float(float val)               {
      if (cat_data_.size() > 0) {
        std::string s(std::to_string(val));
        process_categorical(s.begin(), s.end());
      }
      else {
        number_data_.push_back(val);
      }
    }
    
    /*
     * Passes a floating point datum to the column handler. If categorical
     * data was previously passed to this handler, this datum will be converted
     * to a string and treated as categorical.
     */
    void process_integer(long val)               {
      if (cat_data_.size() > 0) {
        std::string s(std::to_string(val));
        process_categorical(s.begin(), s.end());
      }
      else {
        number_data_.push_back(val);
      }
    }

    /*
     * Passes a categorical datum to the column handler. If numerical data
     * was previously passed to this handler, all previous data passed will
     * be converted to a string.
     */
    template <class Iterator>
    void process_categorical(Iterator begin, Iterator end) {
      if (number_data_.size() > 0) {
        if (begin == end) {
          //std::cout << "{" << std::string(begin, end);
          number_data_.push_back((long)0);
        }
        else {
          //std::cout << "[" << std::string(begin, end);
          convert_to_string();
          std::string key(begin, end);
          cat_data_.push_back((long)get_string_id(key));
        }
      }
      else {
        std::string key(begin, end);
        cat_data_.push_back((long)get_string_id(key));
      }
    }

    /*
      Returns the semantics of this column.
     */
    Semantics get_semantics() const {
      if (cat_data_.size() > 0) {
        return Semantics::STRINGISH;
      }
      else {
        return Semantics::NUMERIC;
      }
    }

    /*
      Returns the type index of the data in this column.
     */
    std::type_index get_type_index() const {
      if (cat_data_.size() > 0) {
        return std::type_index(typeid(std::string));
      }
      else {
        return number_data_.get_type_index();
      }
    }

    std::type_index get_common_type_index(std::type_index &other) const {
      if (cat_data_.size() > 0 || other == std::type_index(typeid(std::string))) {
        return std::type_index(typeid(std::string));
      }
      else {
        return number_data_.get_common_type_index(other);
      }
    }

    template <class T>
    size_t insert_numeric(T *oit) {
      if (cat_data_.size() > 0) {
        throw std::logic_error("expected numeric data");
      }
      size_t to_copy = number_data_.size();
      for (size_t i = 0; i < to_copy; i++) {
        oit[i] = (T)number_data_.get<float>(i);
      }
      return to_copy;
    }

    template <class T, bool Numeric>
    inline typename std::enable_if<std::is_arithmetic<T>::value && Numeric, T>::type get(size_t i) const {
      return number_data_.get<T>(i);
    }

    template <class T, bool Numeric>
    inline typename std::enable_if<std::is_arithmetic<T>::value && !Numeric, T>::type get(size_t i) const {
      return cat_data_.get<size_t>(i);
    }

    const std::vector<std::string> &get_cat_keys() const {
      return cat_keys_;
    }
    
    size_t size() const {
      return (cat_data_.size() > 0) ? cat_data_.size() : number_data_.size();
    }
    
    void clear() {
      number_data_.clear();
      cat_data_.clear();
      cat_ids_.clear();
      cat_keys_.clear();
    }

    size_t get_string(size_t idx) {
      return cat_data_.get<size_t>(idx);
    }
    
    size_t get_string_id(const std::string &key) {
      auto it = cat_ids_.find(key);
      if (it == cat_ids_.end()) {
        std::tie(it, std::ignore) = cat_ids_.insert(std::make_pair(key, cat_ids_.size()));
        cat_keys_.push_back(key);
      }
      return it->second;
    }
    
    /*
     * Converts all floating point data collected by this handler into
     * categorical data.
     */
    void convert_to_string() {
      if (number_data_.size() > 0) {
        for (size_t i = 0; i < number_data_.size(); i++) {
          cat_data_.push_back((long)get_string_id(std::to_string(number_data_.get<float>(i))));
        }
        number_data_.clear();
        number_data_.shrink_to_fit();
      }
    }

    void add_cat_data(const std::string &data) {
      cat_data_.push_back((long)get_string_id(data));
    }

  private:
    std::string column_name_;
    widening_vector_dynamic<uint8_t, int8_t, int16_t, int32_t, int64_t, float> number_data_;
    widening_vector_dynamic<uint8_t, uint8_t, uint16_t, uint32_t, uint64_t>    cat_data_;
    std::unordered_map<std::string, size_t>                                    cat_ids_;
    std::vector<std::string>                                                   cat_keys_;
  };
}
#endif
