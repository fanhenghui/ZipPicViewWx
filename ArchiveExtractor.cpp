#include "ArchiveExtractor.h"
#include "FileEntry.h"
#include <archive.h>
#include <archive_entry.h>
#include <wx/dir.h>
#include <wx/log.h>
#include <wx/stdpaths.h>

Entry *ArchiveExtractor::Create(const wxFileName &filename) {
  wxFile inputFile(filename.GetFullPath());
  auto strTempDir = wxStandardPaths::Get().GetTempDir();
  wxLogMessage(strTempDir);
  wxFileName tempDirName(
      strTempDir + wxFileName::GetPathSeparator() + filename.GetFullName(), "");

  if (tempDirName.DirExists())
    tempDirName.Rmdir();

  if (!tempDirName.Mkdir())
    return nullptr;

  archive_entry *entry;
  int r;

  auto a = archive_read_new();
  archive_read_support_format_all(a);

  if ((r = archive_read_open_fd(a, inputFile.fd(), 10240)))
    return nullptr;

  while ((r = archive_read_next_header(a, &entry)) != ARCHIVE_EOF) {
    if (archive_entry_size(entry) < 0) {
      continue;
    }
    wxFileName entryFilename(archive_entry_pathname(entry));

    wxFile targetFile(tempDirName.GetFullPath() + entryFilename.GetFullName(),
                      wxFile::write);

    archive_read_data_into_fd(a, targetFile.fd());
  }
  archive_read_close(a);
  archive_read_free(a);

  return FileEntry::Create(tempDirName);
}
