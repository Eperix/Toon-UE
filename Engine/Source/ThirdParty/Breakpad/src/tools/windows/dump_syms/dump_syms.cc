// Copyright 2006 Google LLC
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Windows utility to dump the line number data from a pdb file to
// a text-based format that we can use from the minidump processor.

#ifdef HAVE_CONFIG_H
#include <config.h>  // Must come first
#endif

#include <io.h>
#include <stdio.h>
#include <wchar.h>

#include <memory>
#include <string>
#include <fstream>
#include <iostream>

#include "common/linux/dump_symbols.h"
#include "common/windows/pdb_source_line_writer.h"
#include "common/windows/pe_source_line_writer.h"

using google_breakpad::PDBSourceLineWriter;
using google_breakpad::PESourceLineWriter;
using std::unique_ptr;
using std::wstring;

int usage(const wchar_t* self) {
  fprintf(stderr, "Usage: %ws [--pe] [--i] <file.[pdb|exe|dll]>\n", self);
  fprintf(stderr, "Options:\n");
  fprintf(stderr,
          "--pe:\tRead debugging information from PE file and do "
          "not attempt to locate matching PDB file.\n"
          "\tThis is only supported for PE32+ (64 bit) PE files.\n");
  fprintf(stderr,
          "--i:\tOutput INLINE/INLINE_ORIGIN record\n"
          "\tThis cannot be used with [--pe].\n");
  return 1;
}

int wmain(int argc, wchar_t** argv) {
  bool success = false;
  bool pe = false;
  bool handle_inline = false;
  int arg_index = 1;
  while (arg_index < argc && wcslen(argv[arg_index]) > 0 &&
         wcsncmp(L"--", argv[arg_index], 2) == 0) {
    if (wcscmp(L"--pe", argv[arg_index]) == 0) {
      pe = true;
    } else if (wcscmp(L"--i", argv[arg_index]) == 0) {
      handle_inline = true;
    }
    ++arg_index;
  }

  if ((pe && handle_inline) || arg_index == argc) {
    usage(argv[0]);
    return 1;
  }

  wchar_t* file_path = argv[arg_index];
  if (pe) {
    PESourceLineWriter pe_writer(file_path);
    success = pe_writer.WriteSymbols(stdout);
  } else {
    PDBSourceLineWriter pdb_writer(handle_inline);
    if (!pdb_writer.Open(wstring(file_path), PDBSourceLineWriter::ANY_FILE)) {
      std::vector<string> debug_dirs;
      std::wstring s(argv[arg_index]);

      std::streambuf* os = std::cout.rdbuf();
      std::ofstream ofs;

      // Make sure we still have input and output args
      if (arg_index + 1 < argc)
      {
        std::wstring output(argv[arg_index + 1]);
        ofs.open(output, std::ofstream::out);
        if (ofs.is_open()) {
          os = ofs.rdbuf();
        }
      }

      bool enable_multiple_field = false;
      bool handle_inter_cu_refs = true;

      std::ios::sync_with_stdio(false);

      std::ostream out(os);
      SymbolData symbol_data = SymbolData::SYMBOLS_AND_FILES | SymbolData::INLINES;
      google_breakpad::DumpOptions options(symbol_data, handle_inter_cu_refs, enable_multiple_field);

      std::string binary(s.begin(), s.end());
      const char* obj_os = "Linux";

      if (!WriteSymbolFile(binary, binary, obj_os, debug_dirs, options, out)) {
        fprintf(stderr, "Failed to write symbol file.\n");
        ofs.close();
        return 1;
      }

      ofs.close();
      return 0;
    }
    success = pdb_writer.WriteSymbols(stdout);
  }

  if (!success) {
    fprintf(stderr, "WriteSymbols failed.\n");
    return 1;
  }

  return 0;
}
