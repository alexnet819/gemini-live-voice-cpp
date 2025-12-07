# Gemini Live Voice C++

本プロジェクトは、Gemini Live API を活用してリアルタイム音声対話を実現する C++ 製アプリケーションです。

## 機能

*   **リアルタイム音声会話**: マイク入力とスピーカー出力を通じて、Gemini と自然な会話が可能です。
*   **非同期ストリーミング**: Boost.Beast (WebSocket) と miniaudio を使用し、音声とテキストの非同期ストリーミング処理を行います。
*   **Google 検索グラウンディング**: 最新の情報を取得するために、会話に Google 検索の結果を組み込むことができます（設定で有効化）。
*   **柔軟な設定**: 設定ファイル (`config.json`) またはコマンドライン引数で、モデル、音声パラメータ、機能のオン/オフを制御できます。
*   **ダミー音声モード**: 音声デバイスがない環境（サーバーサイドや一部のWSL環境など）でも、テキストベースまたは無音データで動作確認が可能です。

## 必要要件

*   Linux
*   C++23 対応コンパイラ (GCC 12+ / Clang 14+)
*   CMake 3.15 以上
*   依存ライブラリ:
    *   Boost 1.70 以上 (System, Thread)
    *   OpenSSL
    *   ALSA (Linuxでの音声用) または PulseAudio

## ビルド方法

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## 使い方

### 1. APIキーの設定

Google AI Studio で取得した API キーを環境変数に設定するか、実行時にオプションで指定します。

```bash
export GEMINI_API_KEY="あなたのAPIキー"
```

### 2. 実行

```bash
./gemini-voice
```

### コマンドラインオプション

| オプション | 説明 |
|-----------|------|
| `--api-key KEY`, `-k KEY` | APIキーを指定 |
| `--config PATH`, `-c PATH` | 設定ファイルのパス（デフォルト: `./config.json`） |
| `--dummy-audio` | ダミー音声モード（音声デバイス不要） |
| `--enable-search` | Google 検索機能を有効化（設定ファイルより優先） |
| `--help`, `-h` | ヘルプを表示 |

---

## 設定ファイル (config.json)

`config.json` を作成して詳細な設定を行えます。すべての項目は省略可能で、省略時はデフォルト値が使用されます。

### 設定項目一覧

#### モデル設定 (`model`)

| キー | 型 | デフォルト値 | 有効範囲 | 説明 |
|------|-----|-------------|---------|------|
| `name` | string | `"gemini-2.5-flash-native-audio-preview-09-2025"` | Gemini モデル名 | 使用するモデル |
| `temperature` | number | `1.0` | `0.0` ～ `2.0` | 出力のランダム性。高いほど創造的、低いほど確定的 |
| `topP` | number | `0.95` | `0.0` ～ `1.0` | Top-P (nucleus) サンプリング。累積確率で候補を制限 |
| `topK` | integer | `40` | `1` ～ `100` | Top-K サンプリング。上位K個の候補から選択 |

#### 機能設定 (`features`)

| キー | 型 | デフォルト値 | 説明 |
|------|-----|-------------|------|
| `enableSearch` | boolean | `false` | Google 検索によるグラウンディングを有効化。最新情報を取得可能 |
| `inputAudioTranscription` | boolean | `true` | ユーザーの音声入力を文字起こしして表示 |
| `outputAudioTranscription` | boolean | `true` | AIの音声出力を文字起こしして表示 |

#### システム命令 (`systemInstruction`)

| キー | 型 | デフォルト値 | 説明 |
|------|-----|-------------|------|
| `text` | string | `""` (空文字列) | AI への指示プロンプト。会話のペルソナや制約を定義 |

#### 音声設定 (`audio`) - クライアント側設定

| キー | 型 | デフォルト値 | 有効範囲 | 説明 |
|------|-----|-------------|---------|------|
| `inputSampleRate` | integer | `16000` | `8000` ～ `48000` | 入力サンプリングレート (Hz) |
| `outputSampleRate` | integer | `24000` | `8000` ～ `48000` | 出力サンプリングレート (Hz) |
| `chunkSize` | integer | `16000` | `1000` ～ `48000` | 送信チャンクサイズ (サンプル数) |
| `bufferSize` | integer | `24000` | `1000` ～ `96000` | 再生バッファサイズ (サンプル数) |
| `minBufferSize` | integer | `7200` | `1000` ～ `48000` | 最小再生バッファ (サンプル数) |
| `gainFactor` | integer | `5` | `1` ～ `20` | マイク入力の増幅倍率 |

### 設定ファイル例

```json
{
  "model": {
    "name": "gemini-2.5-flash-native-audio-preview-09-2025",
    "temperature": 1.0,
    "topP": 0.95,
    "topK": 40,
    "responseModalities": ["AUDIO"]
  },
  "features": {
    "enableSearch": false,
    "inputAudioTranscription": true,
    "outputAudioTranscription": true
  },
  "systemInstruction": {
    "text": "あなたは親切なアシスタントです。日本語で応答してください。"
  },
  "audio": {
    "inputSampleRate": 16000,
    "outputSampleRate": 24000,
    "chunkSize": 16000,
    "bufferSize": 24000,
    "minBufferSize": 7200,
    "gainFactor": 5
  }
}
```

---

## テストプログラム

`tests/` ディレクトリに機能ごとのテストプログラムが含まれています。

*   `test_audio`: マイク録音と再生のテスト
*   `test_websocket`: Gemini API との WebSocket 通信テスト
*   `test_playback`: 正弦波の再生テスト

## ライセンス

本プロジェクトはサードパーティライブラリとして以下を使用しています。詳細は `THIRD_PARTY_LICENSES.md` を参照してください。

*   **miniaudio**: Audio playback/capture
*   **nlohmann/json**: JSON processing
