Digital Devicesチューナー用BDASpecialプラグイン

【これは何？】
Digital Devices GmbH社製チューナー用のBonDriver_BDA用BDASpecialプラグインです。
BonDriver_BDA.dllと組み合わせて使用します。

【動作環境】
Windows XP/Vista/7/8/8.1/10 (x86/x64)

【対応チューナー】
Digital Devices GmbH社製の下記のチューナーでの動作が確認されているようです。
  Max M4
  Max A8i
他にも対応チューナーは多数あると思われます。

【使い方】
1. BonDriver_BDAの入手
下記URLより、最新バージョンのBonDriver_BDAを入手してください。
https://github.com/radi-sh/BonDriver_BDA/releases
※ BonDriver_BDA改-20190202より前のバージョンでは動作しませんのでご注意ください。

2. 使用するファイル（32ビット用/64ビット用、Windows XP用/Windows Vista以降用、通常版/ランタイム内蔵版）の選択
BonDriver_BDA付属のReadme-BonDriver_BDAを参考に、BonDriver_BDAと同じものを選択してください。

3. Visual C++ 再頒布可能パッケージのインストール
BonDriver_BDA付属のReadme-BonDriver_BDAを参考に、インストールしてください。

4. BonDriverとiniファイル等の配置
・使用するアプリのBonDriver配置フォルダに、BonDriver_BDA.dllをリネームしたコピーを配置
  通常、ファイル名が"BonDriver_"から始まる必要がありますのでご注意ください。
・用意したdllと同じ名前のiniファイルを配置
  下記のサンプルiniファイルを基に作成してください。
    -BonDriver_MaxM4.ini
・DD.dllファイルを配置
  DD.dllファイルはリネームせずに1つだけ配置すればOKです。

(配置例)
  BonDriver_MaxM4_0.dll
  BonDriver_MaxM4_0.ini
  BonDriver_MaxM4_1.dll
  BonDriver_MaxM4_1.ini
  BonDriver_MaxM4_2.dll
  BonDriver_MaxM4_2.ini
  BonDriver_MaxM4_3.dll
  BonDriver_MaxM4_3.ini
  DD.dll

【サポートとか】
・最新バージョンとソースファイルの配布場所
https://github.com/radi-sh/BDASpecial-DD/releases

・不具合報告等
専用のサポート場所はありません。
5chのDTV板で該当スレを探して書込むとそのうち何か反応があるかもしれません。
作者は多忙を言い訳にあまりスレを見ていない傾向に有りますがご容赦ください。

【免責事項】
BDASpecialプラグインおよびBonDriver_BDAや付属するもの、ドキュメントの記載事項などに起因して発生する損害事項等の責任はすべて使用者に依存し、作者・関係者は一切の責任を負いません。

【謝辞みたいなの】
・このBDASpecialプラグインは「Bon_SPHD_BDA_PATCH_2」を基に改変したものです。
・このBDASpecialプラグインのオリジナル処理のほぼ全ては5chの「スカパー! プレミアムをPCで視聴 22」スレの23氏の人柱による情報収集・動作テスト・解析の結果によるものです。
・上記の23氏、作者様、その他参考にさせていただいたDTV関係の作者様、ご助言いただいた方、不具合報告・使用レポートをいただいた方、全ての使用していただいた方々に深く感謝いたします。
