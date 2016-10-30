/*!
  \file        header_splitter.cpp
  \author      Arnaud Ramey <arnaud.a.ramey@gmail.com>
                -- Robotics Lab, University Carlos III of Madrid
  \date        2016/10/29

________________________________________________________________________________

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
________________________________________________________________________________

 */
// C++
#include <algorithm>
#include <ctime>
#include <deque>
#include <fstream>
#include <sstream>
#include <set>
#include <stdio.h>
#include <string>
#include <vector>

inline bool retrieve_file_split(const std::string & filepath,
                                std::vector<std::string> & ans) {
  // open the file
  std::ifstream myfile(filepath.c_str(), std::ios::in);
  if (!myfile.is_open()) {// error while reading the file
    printf("Unable to open file '%s'", filepath.c_str());
    return false;
  }
  std::string line;
  ans.clear();
  while (myfile.good()) {
    getline(myfile, line);
    ans.push_back(line);
  } // end myfine.good()
  myfile.close();
  return true;
}

////////////////////////////////////////////////////////////////////////////////

inline bool save_file_split(const std::string & filepath,
                            const std::vector<std::string> & content) {
  std::ofstream myfile(filepath.c_str());
  if (!myfile.is_open()) { // check if success
    printf("Unable to open file '%s' for writing.", filepath.c_str());
    return false;
  }
  for (unsigned int i = 0; i < content.size(); ++i)
    myfile << content[i] << std::endl;
  return true;
}

////////////////////////////////////////////////////////////////////////////////

inline bool file_exists(const std::string & filename) {
  std::ifstream my_file(filename.c_str());
  return (my_file.good());
}

////////////////////////////////////////////////////////////////////////////////

inline int find_and_replace(std::string& stringToReplace,
                            const std::string & pattern,
                            const std::string & patternReplacement) {
  size_t j = 0;
  int nb_found_times = 0;
  for (; (j = stringToReplace.find(pattern, j)) != std::string::npos;) {
    //cout << "found " << pattern << endl;
    stringToReplace.replace(j, pattern.length(), patternReplacement);
    j += patternReplacement.length();
    ++ nb_found_times;
  }
  return nb_found_times;
}

////////////////////////////////////////////////////////////////////////////////

bool is_only_slashes(const std::string & s) {
  for (unsigned int i = 0; i < s.size(); ++i) {
    if (s[i] != '/')
      return false;
  } // end for i
  return true;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
  if (argc < 3) {
    printf("Synopsis: %s INPUT_HEADER OUTDIR [HEADERTEMPLATE] [MYNAMESPACE]\n", argv[0]);
    printf("Sample: %s  /home/arnaud/vision/processing.h  /tmp  /home/arnaud/Dropbox/code/qtcreator_c++_header_short.h  vision_utils\n", argv[0]);
    return 0;
  }
  std::string input_header = argv[1], outdir = argv[2], headertemplate, mynamespace;
  if (argc >= 4)
    headertemplate = argv[3];
  if (argc >= 5)
    mynamespace = argv[4];
  // read files
  std::vector<std::string> lines, headertemplate_lines;
  if (!retrieve_file_split(input_header, lines)) {
    printf("Could not read INPUT_HEADER '%s'\n", input_header.c_str());
    return -1;
  }
  // read template
  if (headertemplate.size()
      && !retrieve_file_split(headertemplate, headertemplate_lines)) {
    printf("Could not read INPUT_HEADER '%s'\n", input_header.c_str());
    return -1;
  }
  unsigned int nlines = lines.size();
  std::vector<unsigned int> marker_lines;
  std::vector<std::string> marker_files;
  // find cut markers
  for (unsigned int i = 0; i < nlines; ++i) {
    std::string line  = lines[i];
    // "//cut:img2string"
    if (line.size() < 5 || line.substr(0, 5) != "//cut")
      continue;
    marker_lines.push_back(i);
    std::string file = (line.size() >= 6 ? line.substr(6) : "");
    marker_files.push_back(file);
    printf("Marker at line %i : '%s'\n", i, file.c_str());
  } // end for i

  // get some constants
  time_t t = time(NULL);
  tm* timePtr = localtime(&t);
  std::ostringstream syear; syear << 1900 + timePtr->tm_year;
  std::ostringstream smonth; smonth << 1 + timePtr->tm_mon;
  std::ostringstream sday; sday << timePtr->tm_mday;
  std::string year = syear.str(), month = smonth.str(), day = sday.str();

  // for each cut marker, print the appropriate output file
  unsigned int nmarkers = marker_lines.size();
  for (unsigned int i = 0; i < nmarkers; ++i) {
    std::ostringstream line;
    // first add header template
    std::string currfile = marker_files[i];
    if (currfile.empty()) {
      printf("Marker at line %i is empty\n", marker_lines[i]);
      continue;
    }
    std::ostringstream outfilename;
    outfilename << outdir << "/" << currfile << ".h";
    if (file_exists(outfilename.str())) {
      printf("Cannot save file '%s', already exists!\n", outfilename.str().c_str());
      continue;
    }
    std::vector<std::string> currlines = headertemplate_lines;

    // add the header guards
    std::string guard = currfile + "_H";
    std::transform(guard.begin(), guard.end(), guard.begin(), toupper);
    line.str("");
    line << "#ifndef " << guard;
    currlines.push_back(line.str());
    line.str("");
    line << "#define " << guard;
    currlines.push_back(line.str());

    // get the excerpt
    unsigned int lbegin = marker_lines[i] + 1; // +1 to remove the marker
    unsigned int lend = (i == nmarkers - 1 ? nlines : marker_lines[i+1]);
    std::deque<std::string> excerpt_lines;
    for (unsigned int l = lbegin; l < lend; ++l)
      excerpt_lines.push_back(lines[l]);
    // remove the empty beginning and end of the excerpt
    while(is_only_slashes(excerpt_lines.front()))
      excerpt_lines.pop_front();
    while(is_only_slashes(excerpt_lines.back()))
      excerpt_lines.pop_back();

    // add some predefined includes if needed
    std::set<std::string> includes;
    for (unsigned int j = 0; j < excerpt_lines.size(); ++j) {
      if (excerpt_lines[j].find(" cv::") != std::string::npos)
        includes.insert("#include <opencv2/core/core.hpp>");
      if (excerpt_lines[j].find("std::deque") != std::string::npos)
        includes.insert("#include <deque>");
      if (excerpt_lines[j].find("std::map") != std::string::npos)
        includes.insert("#include <map>");
      if (excerpt_lines[j].find("std::ostringstream") != std::string::npos)
        includes.insert("#include <sstream>");
      if (excerpt_lines[j].find("std::string") != std::string::npos)
        includes.insert("#include <string>");
      if (excerpt_lines[j].find("std::vector") != std::string::npos)
        includes.insert("#include <vector>");
    }
    for(std::set<std::string>::const_iterator it = includes.begin();
        it != includes.end(); ++ it)
      currlines.push_back(*it);
    currlines.push_back("");

    // then add the namespace if needed
    if (!mynamespace.empty()) {
      line.str("");
      line << "namespace " << mynamespace << " {";
      currlines.push_back(line.str());
      currlines.push_back("");
    }

    // then add the lines of the excerpt
    for (unsigned int l = 0; l < excerpt_lines.size(); ++l)
      currlines.push_back(excerpt_lines[l]);

    // add the end of the namespace if needed
    if (!mynamespace.empty()) {
      currlines.push_back("");
      line.str("");
      line << "} // end namespace " << mynamespace;
      currlines.push_back(line.str());
      currlines.push_back("");
    }

    // end the include guard
    line.str("");
    line << "#endif // " << guard;
    currlines.push_back(line.str());

    // replace some keywords
    unsigned int ncurrlines = currlines.size();
    for (unsigned int l = 0; l < ncurrlines; ++l) {
      find_and_replace(currlines[l], "%FILENAME%", currfile + ".h");
      find_and_replace(currlines[l], "%YEAR%", year);
      find_and_replace(currlines[l], "%MONTH%", month);
      find_and_replace(currlines[l], "%DAY%", day);
    } // end for l

    // write to file
    printf("Saving file '%s'\n", outfilename.str().c_str());
    save_file_split(outfilename.str(), currlines);
  } // end for i

  return 0;
} // end main
