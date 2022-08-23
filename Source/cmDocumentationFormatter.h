/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <string>

class cmDocumentationSection;

/** Print documentation in a simple text format. */
class cmDocumentationFormatter
{
public:
  void PrintFormatted(std::ostream& os, std::string const& text) const;
  void PrintPreformatted(std::ostream& os, std::string const& text) const;
  void PrintSection(std::ostream& os, cmDocumentationSection const& section);
  void PrintParagraph(std::ostream& os, std::string const& text) const;
  void PrintColumn(std::ostream& os, std::string const& text) const;
  void SetIndent(std::size_t indent) { this->TextIndent = indent; }

private:
  std::size_t TextWidth = 77u;
  std::size_t TextIndent = 0u;
};
