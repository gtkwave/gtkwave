/*
 * Copyright (c) 2025 Leon Wandruschka.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <regex>
#include "json.hpp"

using json = nlohmann::json;


std::map<std::string, std::string> parse_filemap(const json& meta_json);
std::string get_file_id(const std::string& loc);
std::pair<int, int> get_line_range(const std::string& loc);
void json2stems(std::istream& netlist_in, std::istream& meta_in, std::ostream& out);


int main(int argc, char* argv[]) 
{
  if (argc < 4) {
    std::cerr << "Usage: json2stems <meta.json> <netlist.json> <output.stem>\n";
    return 1;
  }

  json j1, j2;
  try 
  {
    std::ifstream f1(argv[1]);
    std::ifstream f2(argv[2]);
    if (!f1.is_open() || !f2.is_open()) throw std::runtime_error("Failed to open input files");
    f1 >> j1;
    f2 >> j2;
  } 
  catch (const std::exception& e) 
  {
    std::cerr << "JSON parse or file error: " << e.what() << std::endl;
    return 1;
  }

  // Determine which file is meta and which is netlist
  std::stringstream meta_buf, netlist_buf;
  if (j1.contains("files") && !j2.contains("files")) 
  {
    meta_buf << j1.dump();
    netlist_buf << j2.dump();
  } else if (!j1.contains("files") && j2.contains("files")) 
  {
    meta_buf << j2.dump();
    netlist_buf << j1.dump();
  } 
  else
  {
    std::cout << "Unable to distinguish meta and netlist JSON." << std::endl;
    return 1;
  }

  std::ofstream out_file(argv[3]);
  if (!out_file.is_open()) 
  {
    std::cout << "Failed to open output file." << std::endl;
    return 1;
  }

  json2stems(netlist_buf, meta_buf, out_file);
  return 0;
}

// Parse file ID to real path mapping from the meta JSON
std::map<std::string, std::string> parse_filemap(const json& meta_json) 
{
  std::map<std::string, std::string> filemap;
  if (meta_json.contains("files")) {
    for (const auto& [id, info] : meta_json["files"].items()) 
    {
      if (info.contains("filename")) 
      {
        filemap[id] = info["realpath"];
      }
    }
  }
  return filemap;
}

// Extract file ID (e.g., "a") from location string like "a,12:0,12:23"
std::string get_file_id(const std::string& loc) 
{
  size_t comma = loc.find(',');
  return (comma != std::string::npos) ? loc.substr(0, comma) : "";
}

// Extract start and end line numbers from location string
std::pair<int, int> get_line_range(const std::string& loc)
{
  std::regex line_re("[a-z],(\\d+):\\d+,(\\d+):\\d+");
  std::smatch match;
  if (std::regex_search(loc, match, line_re) && match.size() >= 3) 
  {
    return {std::stoi(match[1]), std::stoi(match[2])};
  }
  return {0, 0};
}

// Main converter function
void json2stems(std::istream& netlist_stream, std::istream& meta_stream, std::ostream& out) {
  json meta_json, netlist_json;
  meta_stream >> meta_json;
  netlist_stream >> netlist_json;

  auto filemap = parse_filemap(meta_json);

  std::map<std::string, std::string> pointer_to_module_name;
  if (netlist_json.contains("modulesp")) {
    for (const auto& mod : netlist_json["modulesp"]) {
      if (mod.contains("addr") && mod.contains("name")) {
        pointer_to_module_name[mod["addr"]] = mod["name"];
      }
    }
  }

  for (const auto& mod : netlist_json["modulesp"]) {
    if (!mod.contains("name") || !mod.contains("loc")) continue;

    std::string loc = mod["loc"];
    std::string mod_name = mod["name"];
    std::string file_id = get_file_id(loc);
    auto [start_line, end_line] = get_line_range(loc);

    auto it = filemap.find(file_id);
    if (it != filemap.end()) {
      out << "++ module " << mod_name
        << " file " << it->second
        << " lines " << start_line << " - " << end_line << "\n";
    }

    if (mod.contains("stmtsp")) {
      for (const auto& stmt : mod["stmtsp"]) {
        if (stmt.contains("type") && stmt["type"] == "CELL" && stmt.contains("name")) {
          std::string inst_name = stmt["name"];
          std::string target_mod_name = "UNKNOWN";

          if (stmt.contains("modp")) {
            std::string modp = stmt["modp"];
            auto it2 = pointer_to_module_name.find(modp);
            if (it2 != pointer_to_module_name.end()) {
              target_mod_name = it2->second;
            }
          }

          out << "++ comp " << inst_name
            << " type " << target_mod_name
            << " parent " << mod_name << "\n";
        }
      }
    }
  }
}

