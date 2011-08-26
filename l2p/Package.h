//===- Package.h - Lineage II client package reader -----------*- C++ -*-===//
//
// L2Package
//
// This file is distributed under the Simplified BSD License. See LICENSE.TXT
// for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the classes needed to read Lineage II package files.
//
//===----------------------------------------------------------------------===//

#ifndef L2PACKAGE_PACKAGE_H
#define L2PACKAGE_PACKAGE_H

#include "StringRef.h"

#include <cstdint>
#include <memory>
#include <streambuf>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace l2p {

using std::uint64_t;
using std::uint32_t;
using std::int32_t;
using std::int16_t;
using std::uint8_t;

struct Name : public StringRef {
  // MSVS 2010 parses this, but doesn't do anything with it :(.
  // using StringRef::StringRef;

  Name() : StringRef() {}
  Name(const char *Str) : StringRef(Str) {}
  Name(const char *data, size_t length) : StringRef(data, length) {}
  Name(const std::string &Str) : StringRef(Str) {}

  bool operator ==(const Name &other) {
    // A Name always points to a string in the global name table. Because of
    // this we can just compare starting pointers.
    return data() == other.data();
  }
};

struct Index {
  int32_t value;

  operator int32_t() const {
    return value;
  }
};

struct Import {
  Name class_package;
  Name class_name;
  int32_t package;
  Name object_name;
};

struct Export {
  Index class_;
  Index super;
  int32_t package;
  Name name;
  uint32_t flags;
  Index size;
  Index offset;
};

struct GUID {
  uint32_t A, B, C, D;
};

struct GenerationInfo {
  int32_t export_count;
  int32_t name_count;
};

struct Header {
  int32_t tag;
  int16_t file_version;
  int16_t licensee_mode;
  uint32_t package_flags;
  int32_t name_count;
  int32_t name_offset;
  int32_t export_count;
  int32_t export_offset;
  int32_t import_count;
  int32_t import_offset;
  GUID guid;
  int32_t generation_count;
  std::vector<GenerationInfo> generations;
};

class Package {
public:
  // Input operators that cannot be defined in their class because Package
  // depends on them.
  Package &operator >>(Name &n);
  Package &operator >>(Index &i);
  Package &operator >>(Import &i);
  Package &operator >>(Export &e);
  Package &operator >>(GUID &g);
  Package &operator >>(GenerationInfo &g);
  Package &operator >>(Header &h);
  Package &operator >>(char &c) {
    *input >> c;
    return *this;
  }

  operator std::istream &() {
    return *input;
  }

  //! Initalize and verify the root Lineage II path.
  static bool Initialize(StringRef root_path);

  //! Get the package named name under the root. If the package is already
  //! opened, that instance is returned.
  //! returns nullptr if the package does not exist or failed to open.
  static Package *GetPackage(StringRef name);

private:
  Package(StringRef path, Name name);

  //! The list of streambuf filters leading up to the input. input_chain.front()
  //! returns the same value as input.rdbuf().
  //! This generally holds {l2_decrypt_streambuf, std::basic_ifstream<char>}
  std::vector<std::unique_ptr<std::streambuf>> input_chain;
  std::unique_ptr<std::istream> input;
  Name name;
  // Maps package name indicies to global names.
  std::vector<Name> name_map;
  std::vector<Import> import_table;
  std::vector<Export> export_table;
  Header header;

  static std::string GL2RootDir;
  static std::unordered_map<std::string, std::shared_ptr<Package>> GPackages;
  static std::unordered_set<std::string> GNames;
};

template<class ExtractAsT, class StoreToT>
struct ExtractHelper {
  StoreToT &storeTo;
  ExtractHelper(StoreToT &st) : storeTo(st) {}
};

template<class ExtractAsT, class StoreToT, class IStreamT>
IStreamT &operator >>(IStreamT &is, ExtractHelper<ExtractAsT, StoreToT> &eh) {
  ExtractAsT value;
  is >> value;
  eh.storeTo = value;
  return is;
}

// This exists for template argument deduction.
template<class ExtractAsT, class StoreToT>
ExtractHelper<ExtractAsT, StoreToT> Extract(StoreToT &st) {
  return ExtractHelper<ExtractAsT, StoreToT>(st);
}

template<class ExtractSizeAsT, class ExtractElementAsT, class StoreToT>
struct ExtractArrayHelper {
  StoreToT &storeTo;
  ExtractArrayHelper(StoreToT &st) : storeTo(st) {}
};

template<class ExtractSizeAsT, class ExtractElementAsT, class StoreToT, class IStreamT>
IStreamT &operator >>(IStreamT &is, ExtractArrayHelper< ExtractSizeAsT
                                                      , ExtractElementAsT
                                                      , StoreToT> &eah) {
  ExtractSizeAsT size_val;
  is >> size_val;
  int32_t size = size_val;
  eah.storeTo.reserve(size);
  for (int32_t i = 0; i < size; ++i) {
    ExtractElementAsT element;
    is >> element;
    eah.storeTo.push_back(element);
  }
  return is;
}

template<class ExtractSizeAsT, class ExtractElementAsT, class StoreToT>
ExtractArrayHelper<ExtractSizeAsT, ExtractElementAsT, StoreToT> ExtractArray(StoreToT &st) {
  return ExtractArrayHelper<ExtractSizeAsT, ExtractElementAsT, StoreToT>(st);
}

}

#endif
