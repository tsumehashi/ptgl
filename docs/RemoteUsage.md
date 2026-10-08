# Remote Extension の使い方

送信PCで計測やシミュレーションを実行し、別PCのptglまたはブラウザへ3D図形・GUI・プロットを送る。送信SDKはC++17のヘッダーオンリーで、描画ライブラリをリンクしない。受信側は `PTGL_ENABLE_REMOTE_EXTENSION=ON` のときに単一のptglライブラリへ組み込む。

## サンプルの起動

ptglリポジトリをカレントディレクトリにする。受信側のビルドには通常のptglと同じEigen、GLFW、GLEW、OpenGLが必要。独自配置の場合は既存の `EIGEN_DIR`、`GLEW_INCLUDE_DIR`、`GLEW_SHARED_LIBRARY_RELEASE`、`GLFW_INCLUDE_DIR`、`GLFW_LIBRARY` を指定する。

```sh
cmake -S . -B build/Remote -DPTGL_ENABLE_REMOTE_EXTENSION=ON -DPTGL_BUILD_REMOTE_DEMO=ON
cmake --build build/Remote --config Release
```

Windowsではビルドディレクトリに `RunRemoteDemo.cmd` と補助スクリプトを生成する。Release版をビルド後、次のコマンドで送信元と表示を一緒に起動できる。表示を閉じると、この起動操作で作った送信プロセスも終了する。

```powershell
.\build\Remote\RunRemoteDemo.cmd       # 32系列・1 kHz（既定）
.\build\Remote\RunRemoteDemo.cmd 1     # 3DとGUI
.\build\Remote\RunRemoteDemo.cmd 3     # XYと軌跡
.\build\Remote\RunRemoteDemo.cmd 2 -Url ws://192.168.1.10:9002
```

`-Url` を指定すると表示だけを起動する。ローカル送信のポートは `-Port 9003` などで指定でき、`-Port 0` は空きポートを選ぶ。既にポートが使用中の場合はエラーを表示し、既存の送信元へ勝手に接続したり停止したりしない。Debugビルドは `-Configuration Debug` を指定する。

この開発環境では既存の起動スクリプトに合わせ、`build/RunRemoteDemo.cmd`、`build/RunRemote3DDemo.cmd`、`build/RunRemoteXyDemo.cmd` からも起動できる。`build/` はGit管理対象外だが、元のランチャーは `examples/RemoteDemo` にあり、CMake設定時にビルド先へコピーされる。

送信・表示を別々に起動する場合、WindowsのVisual Studioジェネレーターでは2つのターミナルで次を実行する。

```powershell
# 送信PC
.\build\Remote\examples\RemoteDemo\Release\RemotePublisherDemo.exe --port 9002

# 表示PC。192.168.1.10 は送信PCのIPアドレスに置き換える
.\build\Remote\examples\RemoteDemo\Release\RemoteViewerDemo.exe --url ws://192.168.1.10:9002 --view 2
```

NinjaやUnix Makefilesの場合、実行ファイルのパスから `Release/` を除く。WindowsでGLEWやGLFWのDLLを使用する場合は、それらを実行ファイルの隣またはPATH上に配置する。共有版ptglではptgl.dllも必要。

| view | 内容 |
| --- | --- |
| 1 | 回転する箱、球、座標軸とGUI |
| 2 | 32系列・1 kHz・直近60秒の時系列プロット |
| 3 | 静的な円のXYプロットと、直近6,000点の軌跡 |

送信デモは `0.0.0.0:9002` で待ち受ける。ライブラリの既定は `127.0.0.1:9002`。別PCから使う際は送信PCの指定ポートへ接続可能なネットワーク設定が必要。

左側の「Pause source」と「Speed」は送信アプリに返るGUI操作。系列のチェックボックス、ズーム、パン、線幅は表示側の操作。Plot標準のSpaceキーはローカルのsourcePausedを切り替えるが、Remoteの受信は継続する。送信デモの停止には左側のボタンを使う。

## ブラウザで表示

Emscripten環境を有効にしてビルドする。Web側はEigenとEmscriptenのWebGL/GLFWを使用し、WebSocket++/Asioはコンパイルしない。

```sh
emcmake cmake -S . -B build/RemoteWeb -DPTGL_ENABLE_REMOTE_EXTENSION=ON -DPTGL_BUILD_REMOTE_DEMO=ON -DEIGEN_DIR=/path/to/eigen
cmake --build build/RemoteWeb
python -m http.server 8000 --directory build/RemoteWeb/examples/RemoteDemo
```

送信デモを別途起動し、ブラウザで `http://localhost:8000/RemoteViewerDemo.html` を開く。上部の接続先へ `ws://送信PCのIP:9002` を入力し、表示するviewを選んでConnectを押す。8パネルのサンプルはデスクトップ程度の画面幅を想定する。

初版は `ws://` のみ。HTTPで配信する開発用/LAN内構成を対象とし、HTTPSページからのWSS接続、認証、TLS終端は未対応。通信にpthreadやCOOP/COEPヘッダーは不要。

## 送信SDKだけを配布する

この構成ではptgl本体や描画依存を探索しない。インストール処理がSDKのヘッダーとライセンスを1つのディレクトリに集める。

```sh
cmake -S sdk/Remote -B build/RemoteSdk -DPTGL_REMOTE_SENDER_DEMO=ON
cmake --build build/RemoteSdk --config Release
cmake --install build/RemoteSdk --config Release --prefix /absolute/path/to/RemoteSdk
```

配布物の `include/` が唯一必要なincludeルート。`ptgl/Remote`、`websocketpp`、`asio` を含み、ptgl本体や別の実装cppは不要。単一ファイルに結合したヘッダーではなく、ヘッダー一式として配布する。

利用側のCMake:

```cmake
find_package(PtglRemoteSender CONFIG REQUIRED)
add_executable(MyPublisher Main.cpp)
target_link_libraries(MyPublisher PRIVATE ptgl::remoteSender)
```

設定時に `-DCMAKE_PREFIX_PATH=/absolute/path/to/RemoteSdk` を指定する。`ptgl::remoteSender` はINTERFACEターゲットであり、リンクする独自バイナリはない。ptglソースツリーを直接参照する場合は `add_subdirectory(/path/to/ptgl/sdk/Remote RemoteSdk)` でも同じターゲットを使える。

CMakeを使わない場合はC++17、例外、スレッドを有効にする。例えば:

```sh
# GCC/Clang
c++ -std=c++17 -pthread -I/path/to/RemoteSdk/include Main.cpp -o MyPublisher
```

```powershell
# MSVCのDeveloper Command Prompt/PowerShell
cl /std:c++17 /EHsc /utf-8 /I C:\RemoteSdk\include Main.cpp /link ws2_32.lib mswsock.lib
```

OSの通信・スレッド機能へのリンクは必要。アプリが既にAsioを使用する場合はstandalone/header-only設定を統一する。`ASIO_SEPARATE_COMPILATION` と `ASIO_DYN_LINK` は使えない。

## 最小の送信コード

```cpp
#include <ptgl/Remote/RemotePublisher.h>
#include <chrono>
#include <thread>

int main() {
    namespace remote = ptgl::remote;
    remote::RemotePublisher publisher;
    if (publisher.defineView({1, remote::ViewKind::scene3D, "Robot"})
        != remote::PublishResult::Accepted) return 1;

    remote::Frame frame;
    frame.setColor(0.2, 0.5, 0.9)
         .setLineWidth(3)
         .drawBox({0, 0, 0}, {0, 0, 0, 1}, {1, 2, 3});
    // 位置xyz、クォータニオンxyzw、箱の各辺の全長
    if (publisher.submitFrame(1, 1, std::move(frame))
        != remote::PublishResult::Accepted) return 2;
    if (!publisher.listen({"0.0.0.0", 9002})) return 3;

    std::this_thread::sleep_for(std::chrono::seconds(30));
}
```

同じviewId/layerIdへ `submitFrame` を呼ぶとレイヤー全体を置換する。追加描画ではない。`clearLayer` で削除する。公開APIの操作とイベント処理はアプリケーションの1スレッドに集約し、I/OスレッドはSDKが管理する。

## GUIとプロットのAPI

| API | 用途 |
| --- | --- |
| `defineGui(viewId, GuiTree)` | GUI全体を定義または置換。空のtreeで削除 |
| `pollEvent(GuiEvent&)` | GUI操作をアプリ側で取り出す |
| `eventResult(event, accepted)` | trueで値を確定、falseで直前の確定値へ戻す |
| `updateGui(viewId, widgetId, value, text)` | 送信アプリから値を変更 |
| `definePlot(viewId, PlotDefinition)` | 軸・系列・容量を定義。再定義は既存データを消去 |
| `appendSamples(viewId, SampleBatch)` | 時系列のバッチ追加 |
| `setXyData(viewId, XyBatch)` | 静的XY・散布図の点列置換。空配列で消去 |
| `appendXy(viewId, XyBatch)` | 軌跡のリングバッファーへ追加 |

GUIはLabel、Button、CheckBox、Slider、Number、Text、Panelを扱う。CheckBoxはチェック可能なptglボタン、Slider/Numberは整数値。Textの変更はptglのテキスト確定コールバックから返る。Panelの子はptgl標準のフローレイアウトに従う。`updateGui` のtext引数はText入力の値に使う。ラベルや構成の変更は `defineGui` で行う。

GUI操作は `pollEvent` のループで処理し、`eventResult` の戻り値も確認する。確定値は同じviewを購読している全クライアントへ配信される。複数viewで値を共有する場合はアプリ側で他のviewへ `updateGui` を呼ぶ。再接続後やGUI再定義後の古いイベントは受理しない。

`SampleBatch` は以下の形式:

- `firstIndex`: 先頭サンプルの単調増加インデックス。
- `channels`: PlotDefinitionと同じチャンネル数。
- `timesNs`: 各行のint64ナノ秒時刻。非負で厳密に増加する。
- `values`: float32を行優先で格納。`timesNs.size() * channels` 要素。

初回サンプルの時刻を基準として表示秒に変換する。欠測値はNaN、通信中の欠落はfirstIndexの飛びで示す。過去方向の時刻・重複インデックスは拒否する。10 ms程度ずつのバッチ送信を推奨する。SDK呼び出しには確保・コピー・ロックがあるため、ハードリアルタイム計測スレッドから直接呼ぶ設計にはしない。

静的XYは `SeriesDefinition::capacity` 点まで。軌跡は同じcapacityをリング容量として使う。`XyPoint::breakBefore` は指定点の直前で線を分断する。`appendXy` のfirstIndexが飛んだ場合も分断する。

## 受信を自分のアプリへ組み込む

```cpp
#include <ptgl/Extension/Remote/RemoteGraphicsView.h>

int main() {
    ptgl::ext::remote::RemoteGraphicsView view(1);
    view.setWindowSize(1280, 800);
    if (!view.connect("ws://192.168.1.10:9002")) return 1;
    view.initialize();
    view.execute();
}
```

Plotではヘッダーと型を `RemotePlotGraphicsView` にする。受信側のリンク先は通常どおり `ptgl::ptgl`。`receiver().isConnected()`、`receiver().lastError()`、`binding().appliedRevision()` を状態表示に利用できる。`connect()` は接続開始の成否であり、接続完了は非同期。

1つのRemoteReceiverは1つのviewを購読する。複数viewには複数receiver/viewを用意し、各ビューを既存ptglの実行規則に従って駆動する。任意の既存GraphicsViewへ組み込む場合は `RemoteReceiver` と `ViewBinding` を保持し、描画スレッドの前処理で `applyPending()` を呼ぶ。bindingはviewより先に破棄する。

## 配信と再接続の意味

`PublishResult::Accepted` はSDKがデータを受け付けたことを示す。到着・描画完了の保証ではない。図形、GUI、Plot定義、静的XYは未接続でも保持する。待ち受けを止めた状態でのライブデータ追加はClosed、待ち受け中に閲覧者がいない場合はAcceptedとなるが履歴は残さない。

低速クライアントの数値バッチはキュー上限で破棄し、`statistics().droppedSamples` に集計する。GUI・状態定義が詰まった場合はそのクライアントを切断する。初版のQueueFull列挙値は将来用で、現在のAPIは接続ごとの詰まりをQueueFullとして返さない。

切断後は約1秒間隔で再接続する。現在の図形・GUI確定値・定義・静的XYを復元し、時系列・軌跡は空から再開する。Plotの再定義・再接続ではローカルのズーム・表示設定も初期化される。

## 検証と現時点の範囲

```sh
cmake -S . -B build/Remote -DPTGL_ENABLE_REMOTE_EXTENSION=ON -DPTGL_BUILD_REMOTE_TESTS=ON -DPTGL_BUILD_PLOT_TESTS=ON
cmake --build build/Remote --config Release
ctest --test-dir build/Remote -C Release --output-on-failure
```

送信SDK単体のテストは `PTGL_REMOTE_SENDER_TESTS=ON`。EmscriptenのテストはNodeを `CMAKE_CROSSCOMPILING_EMULATOR` に設定する。

Windows/MSVCとEmscriptenで動作を検証した初版。Linux/macOS、GCC/Apple Clang、Autobahn適合性試験、長時間の負荷試験は今後の検証対象。3Dメッシュ・画像・フォントの転送、認証/TLS、中継、描画適用のACK、単一キャンバスでの3D/Plot複合配置は未実装。通信形式はまだ安定版として固定していない。

内部構造と上限は [Remote Extension 設計](RemoteDesign.md)、同梱ライブラリとライセンスは [thirdparty一覧](../thirdparty/Remote/Readme.md) を参照。
