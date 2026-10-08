# Remote transport dependencies

Vendored from the upstream release archives, with no source modifications.

| Component | Tag | Git object returned for tag | Archive SHA256 |
| --- | --- | --- | --- |
| WebSocket++ | 0.8.2 | 0e4241727c199e208ceb41134e840ba9968cf181 | 6ce889d85ecdc2d8fa07408d6787e7352510750daa66b5ad44aacb47bea76755 |
| standalone Asio | asio-1-28-0 | d079d99d8ad6bce035c4f332e1fcb900b3ab9a51 | 226438b0798099ad2a202563a83571ce06dd13b570d8fded4840dbc1f97fa328 |

Sources: https://github.com/zaphoyd/websocketpp and https://github.com/chriskohlhoff/asio.
Only the include trees and license/readme files needed by the SDK are retained.
See WebSocket++ COPYING (including its embedded third-party notices) and Asio LICENSE_1_0.txt.
Native sender and receiver share these headers. They are compiled into the application and ptgl respectively.
The browser receiver uses Emscripten's WebSocket API and does not compile these headers.
TLS, deflate compression, and the obsolete WebSocket protocol drafts are not part of the ptgl protocol.
The release pair is pinned for reproducible C++17 builds; validation performed on this repository is recorded in the Remote documentation.
