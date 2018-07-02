#ifndef VERSION_H
#define VERSION_H

#include <string>

namespace Version {
  const int MAJOR = 0;
  const int MINOR = 1;
  const int PATCH = 0;
  const std::string Commit = "dce2cf839c9fd84eea48003b1760382d7dd72991";
  const std::string Url = "https://gitlab.rschluessler.com/BrillouinMicroscopy/LQTControl/commit/dce2cf839c9fd84eea48003b1760382d7dd72991";
  const std::string Date = "2018-05-25 10:02:41 +0200";
  const std::string Subject = "Don-t-set-voltage-to-zero-on-startup";
  const std::string Author = "Raimund Schlüßler";
  const std::string AuthorEmail = "raimund.schluessler@tu-dresden.de";
  const bool VerDirty = true;
}
#endif // VERSION_H
