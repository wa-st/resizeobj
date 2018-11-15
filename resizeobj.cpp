// resizeobj.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <fcntl.h>
#include <io.h>

#include "utils.h"
#include "paknode.h"
#include "ImgConverter.h"
#include "ImgUpscaleConverter.h"
#include "TileConverter.h"

const char *RESIZEOBJ_SIGNATURE = "/resizeobj";

enum ConvertMode
{
	cmNoConvert,
	cmDownscale,
	cmUpscale,
	cmSplit,
};

class ResizeObj
{
private:
	ImgConverter m_ic;
	ImgUpscaleConverter m_iuc;
	TileConverter m_tc;
	std::string m_addonPrefix;
	bool m_headerRewriting;
	ConvertMode m_convertMode;
	std::string m_newExt;

	void convertPak(PakFile &pak) const;
	void convertAddon(PakNode *addon) const;
public:
	ResizeObj();
	void convertStdIO() const;
	void convertFile(std::string filename) const;

	void setAntialias(int val) { m_ic.setAlpha(val); };
	void setAddonPrefix(std::string val) { m_addonPrefix = val; };
	void setNewExtension(std::string val) { m_newExt = val; };
	void setHeaderRewriting(bool val) { m_headerRewriting = val; };
	void setConvertMode(ConvertMode val) { m_convertMode = val; };
	void setSpecialColorMode(SCConvMode val) { m_ic.setSpecialColorMode(val); };
	void setTileNoAnimation(bool val) { m_tc.setNoAnimation(val); };
	void setNewTileSize(int val) { m_ic.setNewTileSize(val); };
};

/// アドオン名変更処理の対象外とする形式一覧.
const char *RENAME_NODE_EXCEPTIONS[] = {
	"GOOD",
	//	"SMOK",
};


bool isRenameNode(const char *name)
{
	for (int i = 0; i < lengthof(RENAME_NODE_EXCEPTIONS); i++)
	{
		if (strncmp(RENAME_NODE_EXCEPTIONS[i], name, NODE_NAME_LENGTH) == 0)
			return false;
	}
	return true;
}

/**
	アドオン名の先頭にposfixを追加する.

	アドオン名定義用のTEXTノードとアドン参照用のXREFノードの先頭にprefixを追加する。
書籍:
pak64:  Buecher
pak128: Bucher
*/
void renameObj(PakNode *node, std::string prefix)
{
	if (!isRenameNode(node->type().data()))
		return;

	// XREFノードの場合は変換する
	if (node->type() == "XREF")
	{
		PakXRef *x = &node->dataP()->xref;

		if (isRenameNode(&x->type[0]))
		{
			if (node->data()->size() > sizeof(PakXRef))
			{
				std::vector<char> *dat = node->data();
				dat->insert(dat->begin() + offsetof(PakXRef, name),
					prefix.begin(), prefix.end());
			}
		}
	}

	// 最初の子ノードがTEXTの場合はそれをアドオン名とみなして変換する。
	if (node->begin() != node->end() && (*node->begin())->type() == "TEXT")
	{
		std::vector<char> *dat = (*node->begin())->data();
		dat->insert(dat->begin(), prefix.begin(), prefix.end());
	}

	for (PakNode::iterator it = node->begin(); it != node->end(); it++)
		renameObj(*it, prefix);
}

std::string getAddonName(PakNode *node)
{
	if (node->type() == "TEXT")
		return node->dataP()->text;
	else if (node->length() > 0)
		return getAddonName(node->at(0));
	else
		return "";
}

ResizeObj::ResizeObj()
{
	m_tc.imgConverter(&m_ic);
}

void ResizeObj::convertAddon(PakNode *addon) const
{
	std::clog << "    " << getAddonName(addon) << std::endl;

	switch (m_convertMode)
	{
	case cmSplit:
		if (addon->type() == "SMOK")
		{
			m_tc.convertAddon(addon);
		}
		else if (addon->type() == "BUIL" || addon->type() == "FACT"
			|| addon->type() == "FIEL")
		{
			m_tc.convertAddon(addon);
		}
		break;

	case cmDownscale:
		m_ic.convertAddon(addon);
		break;

	case cmUpscale:
		m_iuc.convertAddon(addon);
		break;
	}

	if (m_addonPrefix.size())
	{
		renameObj(addon, m_addonPrefix);
	}
}

void ResizeObj::convertPak(PakFile &pak) const
{
	if (m_headerRewriting)
		pak.setSignature(pak.signature() + RESIZEOBJ_SIGNATURE);

	PakNode &root = pak.root();
	for (PakNode::iterator it = root.begin(); it != root.end(); it++)
		convertAddon(*it);
}

void ResizeObj::convertStdIO() const
{
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);

	PakFile pak;
	pak.load(std::cin);
	convertPak(pak);
	pak.save(std::cout);
}

void ResizeObj::convertFile(std::string filename) const
{
	std::clog << filename << std::endl;


	PakFile pak;
	pak.loadFromFile(filename);

	if (pak.signature().find(RESIZEOBJ_SIGNATURE) != std::string::npos)
	{
		std::clog << "    skipped." << std::endl;
		return;
	}

	convertPak(pak);

	if (m_newExt != "") filename = changeFileExtension(filename, m_newExt);

	pak.saveToFile(filename);
}

void printOption()
{
	std::clog <<
#ifdef _DEBUG
		"DEBUG "
#endif
		"resizeobj ver 1.5.0 beta by wa\n\n"
		"resizeobj [オプション...] 対象ファイルマスク...\n"
		"オプション:\n\n"
		" -A=(0...100) 画像縮小時のアンチエイリアス量\n"
		"                0: アンチエイリアス無し\n"
		"              100: アンチエイリアス強め(既定値)\n"
		" -S=(0...2)   画像縮小時の特殊色(発光色・プレイヤー色)の扱い\n"
		"                0: 特殊色を使用せず縮小し、昼間の見た目を優先する\n"
		"                1: 縮小元エリアの左上が特殊色の場合はそれを出力(既定値)\n"
		"                2: 縮小元エリアで特殊色が半数以上使用されている場合はそれを出力\n"
		" -W=(16...255) 画像変換後のタイルサイズを指定する。既定値は「64」"
		"\n"
		" -K           原寸大モード\n"
		" -KA          原寸大モードで建築物を変換する際にアニメーションを取り除く\n"
		"\n"
		" -X           拡大モード\n"
		"\n"
		" -E=(ext)     出力するファイルの拡張子。既定値は「.64.pak」\n"
		" -T=(text)    アドオン名の先頭に指定されたtextを追加する\n"
		" -N           ファイルヘッダの書き換えを行わない\n"
		"\n"
		" -D           エラー時にダイアログを表示しない\n"
		" -? , -H      この画面を出力する"
		<< std::endl;
}

template<class T> T optToEnum(const std::string &text, int high, const char *opt)
{
	int value = atoi(text.c_str());
	if (value < 0 || high < value)
	{
		std::ostringstream os;
		os << opt << "オプションの有効範囲は0～" << high << "です。: " << text;
		throw std::runtime_error(os.str());
	}
	return static_cast<T>(value);
}

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
#endif

	bool isShowErrorDialog = true;

	try
	{
		//std::locale::global(std::locale(""));

		ResizeObj ro;
		ro.setAntialias(100);
		ro.setHeaderRewriting(true);
		ro.setSpecialColorMode(scmTOPLEFT);
		ro.setAddonPrefix("");
		ro.setConvertMode(cmDownscale);
		ro.setNewExtension(".64.pak");
		ro.setTileNoAnimation(false);

		std::vector<std::string> files;
		for (int i = 1; i < argc; i++)
		{
			std::string key, val;
			parseArg(argv[i], key, val);
			if (key == "")
			{
				if (val.find_first_of("*?") == std::string::npos)
					files.push_back(val);
				else
					fileList(files, val);
			}
			else if (key == "K")
			{
				ro.setConvertMode(cmSplit);
			}
			else if (key == "X")
			{
				ro.setConvertMode(cmUpscale);
				ro.setNewExtension(".128.pak");
			}
			else if (key == "KA")
			{
				ro.setTileNoAnimation(true);
			}
			else if (key == "A")
			{
				ro.setAntialias(optToEnum<int>(val, 100, "A"));
			}
			else if (key == "S")
			{
				ro.setSpecialColorMode(optToEnum<SCConvMode>(val, 2, "S"));
			}
			else if (key == "W")
			{
				int ts = optToEnum<int>(val, 0xFFFF, "W");
				if (ts % 8 != 0) throw std::runtime_error("タイルサイズは8の倍数に限ります。");
				ro.setNewTileSize(ts);
			}
			else if (key == "N")
			{
				ro.setHeaderRewriting(false);
			}
			else if (key == "T")
			{
				ro.setAddonPrefix(val);
			}
			else if (key == "E")
			{
				if (val != "") ro.setNewExtension(val);
			}
			else if (key == "D")
			{
				isShowErrorDialog = false;
			}
			else if (key == "H" || key == "?")
			{
				printOption();
				return 0;
			}
			else
			{
				std::clog << "無効なオプションです : " << key << std::endl;
				printOption();
				return 1;
			}
		}
		if (files.empty())
		{
			printOption();
			return 0;
		}

		for (std::vector<std::string>::iterator it = files.begin(); it != files.end(); it++)
		{
			if (*it == "con")
				ro.convertStdIO();
			else
				ro.convertFile(*it);
		}

	}
	catch (const std::exception &e)
	{
		std::cerr << "Error : " << e.what() << std::endl;
		if (isShowErrorDialog)
			showErrorDialog(e.what(), "Error");
		return 1;
	}
	return 0;
}

