# Remote Extension 設計

更新日: 2026-10-08。ヘッダーオンリー送信SDKとptgl内の受信Extensionを実装した初版。利用手順とコンパイル可能な例は [Remote Extension の使い方](RemoteUsage.md) を参照。

3D基本図形、基本GUIと操作イベント、時系列・静的XY・散布図・軌跡をWebSocketのバイナリメッセージで送る。受信した命令を描画スレッドでptglのローカルオブジェクトへ変換する。

## 依存と配置

| 区分 | 配置 | 名前空間 | 依存 |
| --- | --- | --- | --- |
| 送信SDK・通信形式 | `ptgl/Remote` | `ptgl::remote` | 標準C++17、WebSocket++、standalone Asio、OS通信機能 |
| 受信Extension | `ptgl/Extension/Remote` | `ptgl::ext::remote` | 共通形式、Core、GUI、Plot、通信実装 |
| 外部コード | `thirdparty/Remote` | 上流の名前空間 | 固定版のヘッダーとライセンス |
| SDK配布 | `sdk/Remote` | CMake `ptgl::remoteSender` | INTERFACEターゲットのみ |
| 受信ビルド | `cmake/PtglRemote.cmake` | 単一ptglバイナリに取り込み | Extension有効時のみ |

送信側はEigen、GLFW、GLEW、OpenGL、ptgl本体に依存しない。別の実装cppやIMPLEMENTATIONマクロも不要。全定義をinline/テンプレート/インスタンスメンバーとし、複数翻訳単位からincludeできる。SDKインストール時に上流ヘッダーも1つのincludeルートへ配置する。

Core、GUI、PlotからRemoteへの依存は追加しない。Plotには通信欠落等にも利用できる `breakBeforeNextSample()` のみを追加した。自前のファイル名はPascalCase、関数はlowerCamelCase、thirdpartyは上流の名前を維持する。

```text
ptgl/Remote/
    Types.h, Frame.h, Gui.h, Plot.h, Protocol.h, RemotePublisher.h
    Detail/Codec.h, Detail/WebSocket.h
ptgl/Extension/Remote/
    RemoteReceiver.h/.cpp, ViewBinding.h/.cpp
    RemoteGraphicsView.h/.cpp, RemotePlotGraphicsView.h/.cpp
sdk/Remote/
    CMakeLists.txt, PtglRemoteSenderConfig.cmake.in
examples/RemoteDemo/
    Publisher.cpp, Viewer.cpp, WebShell.html
tests/Remote/
    ProtocolTests.cpp, SenderTests.cpp, SecondTranslationUnit.cpp, ReceiverTests.cpp
```

## 通信ライブラリ

| 場所 | 採用実装 |
| --- | --- |
| ネイティブ送信 | WebSocket++ 0.8.2 と standalone Asio 1.28.0 |
| ネイティブ受信 | 同じWebSocket++/Asio |
| Emscripten受信 | Emscripten WebSocket API |

受信も送信と同じライブラリを使い、当初候補のlibwebsocketsは追加しない。ヘッダーを受信実装cpp内でコンパイルするため、別の通信バイナリを公開リンク先に増やさない。ブラウザ版はWebSocket++/Asioをコンパイルしない。

WebSocket++とAsioはヘッダーオンリー構成を利用し、Boost、TLS、deflate圧縮は必須にしない。上流の固定版ヘッダー・ライセンス・取得元・SHA256を [thirdparty/Remote](../thirdparty/Remote/Readme.md) に収録し、ビルド中のダウンロードは行わない。Windows/MSVCとEmscriptenを検証済みで、Linux/macOSでの採用判定は継続する。

参考: [WebSocket++](https://github.com/zaphoyd/websocketpp)、[Asio](https://github.com/chriskohlhoff/asio)、[Emscriptenの通信API](https://emscripten.org/docs/porting/networking.html)。

## 接続とスレッド

送信PCがWebSocketサーバー、表示PCまたはブラウザがクライアントになる。HTML/JS/Wasmは独立したHTTPサーバーから配信する。送信SDKにHTTPファイルサーバーは含めない。

```text
計測・シミュレーションのアプリスレッド
  -> RemotePublisherの状態と接続別キュー
  -> 内部I/OスレッドのWebSocketサーバー
  <=> ネイティブまたはブラウザのWebSocketクライアント
  -> RemoteReceiverの上限付き受信キュー
  -> 描画スレッドのViewBinding
  -> GraphicsView / GUI / PlotGraphicsView
```

送信APIと `pollEvent` は1つのアプリスレッドから呼ぶ。通信コールバックからアプリの処理を直接呼ばない。ネイティブの受信は内部I/Oスレッド、Web受信はブラウザのコールバックを使う。Web版の通信にpthreadは不要。

1接続は1つのviewIdを購読する。複数ビューには別々のRemoteReceiverを作る。同じviewへの複数閲覧者を許可し、送信者から無制限にOSウィンドウを生成しない。

`RemoteGraphicsView` は3DとGUI、`RemotePlotGraphicsView` はPlotとGUIを扱う。既定はCallingThreadドライバーで、任意のGraphicsDriverを渡すコンストラクターもある。前処理で `ViewBinding::applyPending()` を実行してから既存の前処理へ進む。PlotのsourcePaused状態でも通信処理は継続する。

I/OスレッドはGL、Widget、Figureに触れない。終了時はビューの停止を待ち、接続を止め、bindingを破棄する。GUIコールバックはbindingの寿命トークンを確認する。

## バイナリ形式

WebSocketのバイナリメッセージ1つに、ptglパケット1つを格納する。WebSocketのマスキング・フラグメント・ping/pong・closeは通信ライブラリに任せる。[RFC 6455](https://datatracker.ietf.org/doc/html/rfc6455)

| フィールド | 型 |
| --- | --- |
| magic | 4 bytes、ASCIIのPTGR |
| major / minor | 各uint16、初版は1 / 0 |
| type / flags | 各uint16、flagsは0 |
| headerBytes | uint32、48 |
| payloadBytes | uint32 |
| viewId | uint32 |
| sessionId | uint64 |
| sequence | uint64 |
| revision | uint64 |

整数はlittle-endian、実数はIEEE 754 float32/float64、文字列は長さ付きUTF-8、boolは0/1のuint8。C++構造体、ポインター、size_t、enumの生メモリーを送らない。UTF-8、有限値、サイズ、積のoverflow、ID、型を検証する。未知の版・type・flags・描画opcodeは初版では拒否する。

Helloには購読viewIdと受け入れ可能なメッセージ上限を含め、WelcomeでsessionIdを受け取る。機能ビットの交渉や未知のoptionalコマンドのスキップは未実装。

| 分類 | メッセージ |
| --- | --- |
| 接続 | hello、welcome、flowAck |
| 状態同期 | snapshotBegin、snapshotEnd、defineView |
| 3D | replaceLayer、clearLayer |
| GUI | defineGui、guiValue、guiEvent |
| Plot | definePlot、appendSamples、setXyData、appendXy |

errorのtype番号は予約済み。プロトコル違反は切断し、受信側はlastErrorを保持する。通信越しのApplied通知、個別のDeleteGui/ClearPlot、機能交渉は今後の拡張。GUIの空定義、Plot再定義、空の静的XY点列でそれぞれを消去できる。

sequenceは接続・方向ごとに実際に送信するとき採番する。revisionは発行側で採番するが、初期状態の各パケットやGUI値には元の定義revisionが含まれるため、受信順の単調性をrevisionで判定しない。GUI操作には対象の定義revisionとeventIdを持たせ、発行側でpeerIdと対応づける。

## 3DとGUI

Frameはレイヤー全体の自己完結した描画記録。色、線幅、点サイズ、材質、点・線・箱・球・円柱・軸、変換スタックを扱う。クォータニオンはxyzw。変換のpush/pop均衡、深さ64、寸法、有限値、色を検証してからローカルRender3DItemへ変換する。内部のserialize()形式を通信へ流用しない。

未送信の同一レイヤーの完全フレームは置換できる。ただしclearLayerや定義変更等の順序境界を越えて置換しない。異なる命令を1つのトランザクションにするAPIは初版に含めず、再接続時のsnapshotを一括適用する。

GUI定義はWidgetのID、種類、親、サイズ、状態、値を持つ。親Panelは子より前に定義する。親子の循環・重複IDを拒否する。白系テーマを使い、ルート位置・寸法を受信側のDPIで変換する。Panel内部は既存ptglのレイアウトを使う。

ローカルの入力反応の後、アプリがpollEventで操作を受け取りeventResultで値を確定または元に戻す。確定値は同じviewの閲覧者へ配信する。受信値の適用ではコールバックを再発火させない。再定義・切断後の古い操作は破棄する。切断中と新しいsnapshotの適用前はGUIを無効にする。再接続で確定値を復元する。

## Plotデータ

時系列は定義とバッチを分離する。バッチはfirstIndex、channels、各行のint64ナノ秒時刻、行優先float32値配列を持つ。固定周期も各行に時刻を格納する。時刻は非負で厳密に増加し、Inf値は禁止、NaN値は欠測として許可する。

受信後、最初の時刻を引いてdouble秒へ変換する。大きな時刻によって隣接サンプルが同じdoubleへ丸められる場合も拒否する。firstIndexの欠落は `breakBeforeNextSample()` で分断し、有効サンプルをNaNへ置き換えない。リングの周回、間引き、未間引きの表示で境界を保持する。

時系列と軌跡は容量固定のリング、静的XYは容量内の点列全体の置換。描画の間引き・ライブズーム・パン・系列表示切替・線幅変更は既存Plotの機能を利用する。送信側に時系列・軌跡の履歴を保持しない。再接続では新しい空のFigureから開始するため、ローカルのPlot閲覧状態も初期化される。

32系列・1 kHzなら値は128,000 bytes/s、8バイト時刻を加えると136,000 bytes/s。10 ms単位のサンプルに48バイトヘッダーと16バイトのバッチ情報を加えると約142,400 bytes/sで、WebSocket/TCP/IPの分は別。これは計算上の値であり、遅延・fpsの保証ではない。

## 上限と低速クライアント

| 対象 | 初版の上限 |
| --- | --- |
| 1パケットのペイロード | 4 MiB |
| 接続あたりの未確認バイト | 8 MiB |
| 接続あたりのライブ送信キュー / 受信キュー | 各16 MiB、各4,096パケット |
| 発行側の保持状態 | 合計64 MiBのシリアライズ済みデータ |
| 受信側の3D記録 | 1モデルあたり32 MiB |
| 受信側のPlot | 容量から求める保守的な推定量で1モデル64 MiB以下 |
| view / client / レイヤー | 8 / 8 / viewあたり128 |
| GUI / 文字列 | viewあたり256個 / 1文字列4,096バイト |
| Plot系列 / 1XYバッチ | 128系列 / 100,000点 |
| GUIイベントキュー | 1,024件 |

Plotの推定量には時系列、要約木、静的XYと軌跡の容量を含める。静的XYは宣言容量を超える置換を拒否する。初期同期中は現在モデルと仮モデルの両方を持ち、変換中の一時コピーもある。表の値は全プロセスの厳密なメモリー上限ではなく、描画キャッシュ、アロケーター、GUI、通信実装、OS、GPUの消費は別に発生する。

受信キューをdrainしたバイト数をFlowAckで返し、未確認枠とWebSocket内部の未送信量を見て送信を止める。FlowAckは通信枠の解放であり、モデル適用・GPU描画・画面表示の完了ではない。

snapshotは接続時点の保持パケットへの不変参照を捕捉し、8 MiBの通信枠に従って順に送る。16 MiBのライブキューとは分けるため、大きな状態でもFlowAckを返しながら同期できる。snapshot中の後続ライブ更新は接続別キューへ入る。

キューが詰まると数値バッチは当該クライアントだけで破棄し、統計のdroppedSamplesを加算する。次のfirstIndexから欠落を表示する。GUI・定義更新は黙って捨てず、当該クライアントを切断する。未確認データのタイムアウトは10秒。遅い閲覧者のために他の閲覧者のキューを停止しない。

公開APIのAcceptedは受け付けであり到達保証ではない。TooLarge、InvalidArgument、Closedを返す。QueueFull列挙値は拡張用で、現在は接続ごとのキュー上限を公開APIのQueueFullへ変換しない。詳細な接続別統計、制御専用予約枠、適用ACKは今後の拡張とする。

## 再接続と状態同期

接続後はWelcome、SnapshotBegin、保持状態、SnapshotEndの順に送る。受信側は仮モデルを検証・構築し、SnapshotEndで描画フレーム境界にて現在モデルと置換する。破損したsnapshotで表示中のモデルを部分的に消さない。

保持するのはビュー定義、最新3Dレイヤー、GUI確定値、Plot定義、静的XY。時系列・軌跡の履歴と未確認クリックは再送しない。切断後は約1秒間隔で再試行する。再接続したpeerやGUI再定義前のイベントを現在の操作として適用しない。

## CMakeと検証範囲

`PTGL_ENABLE_REMOTE_EXTENSION` は既定OFF。OFF時はRemoteのcppをGLOBから除き、通信ヘッダーの探索・コンパイル・リンクを行わない。ON時は受信の4つのcppをptglに追加する。ネイティブの通信型は受信の公開ヘッダーへ出さず、Web側は `-lwebsocket.js` を使う。

送信SDKは独立したCMake入口とインストール設定を持ち、`find_package(Ptgl)` を呼ばない。`ptgl::remoteSender` はヘッダーとOSリンク設定を伝える。第三者コードのライセンスとptglのMITライセンスをSDKへ同梱する。

自動テストはパケット境界・不正入力・メモリー推定・複数翻訳単位・16 MiBを超える初期同期・実WebSocket往復・GUI確定値・欠落・再接続を対象とする。WindowsネイティブとEmscriptenのビルド、実ブラウザとネイティブビューで3D/時系列/XYの接続、SDK単独のインストールと外部プロジェクトからの利用を検証する。

初版の外に残す項目は、Linux/macOS実機検証、Autobahn等によるWebSocket適合性試験、長時間/低速回線での負荷計測、TLS・認証・中継、メッシュ・画像・フォントの動的転送、3D/Plot複合キャンバス、適用ACKと詳細な接続別統計。通信形式は初版段階であり、安定版の互換性保証はまだ行わない。
