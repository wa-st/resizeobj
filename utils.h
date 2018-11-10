#pragma once

#define lengthof(ary) (sizeof(ary)/sizeof(ary[0]))

/** ポインタ型から別のポインタ型へキャスト.
 * http://www.geocities.jp/ky_webid/cpp/language/024.html
 */
template <class T>
inline T pointer_cast(void* p)
{
	return static_cast<T>(p);
}
template <class T>
inline T pointer_cast(const void* p)
{
	return static_cast<T>(p);
}

/*
  エラーチェック付きのポインタのdynamic_cast
*/
template<class T, class U> 
inline T object_cast(U* o)
{
	T v = dynamic_cast<T>(o);
	if(!v && o) throw std::bad_cast();
	return v;
}

/**  コマンドラインオプションを解析する.
 *   options = (-|--|/)[A-Za-z90-9_]+([:=]?.+)?
 *   @param arg 解析対称
 *   @param[out] key   オプションの場合は大文字でそのキー名を、ファイル名なら空文字列を格納
 *   @param[out] value 値ありオプションの場合はそれ、値無オプションなら空文字列、ファイル名ならそれを格納
 */
void parseArg(const char *arg, std::string &key, std::string &value);

/** ファイル名拡張子の変更を行う.
 *
 *  @param path 変更元。変更されるのは最後の"."以降
 *  @param ext "."も含めて指定する。""を使用すれば拡張子の除去となる。
 */
std::string changeFileExt(const std::string path, const std::string ext);

/** ディレクトリ直下のファイル一覧を作成する. */
void fileList(std::vector<std::string> &entries, const std::string mask);
