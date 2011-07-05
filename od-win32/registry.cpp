
#include "sysconfig.h"
#include "sysdeps.h"

#include <windows.h>
#include <shlwapi.h>
#include "win32.h"
#include "registry.h"
#include "crc32.h"

static int inimode = 0;
static TCHAR *inipath;
#define PUPPA L"eit�t�oo"

static HKEY gr (UAEREG *root)
{
	if (!root)
		return hWinUAEKey;
	return root->fkey;
}
static TCHAR *gs (UAEREG *root)
{
	if (!root)
		return L"WinUAE";
	return root->inipath;
}
static TCHAR *gsn (UAEREG *root, const TCHAR *name)
{
	TCHAR *r, *s;
	if (!root)
		return my_strdup (name);
	r = gs (root);
	s = xmalloc (TCHAR, _tcslen (r) + 1 + _tcslen (name) + 1);
	_stprintf (s, L"%s/%s", r, name);
	return s;
}

int regsetstr (UAEREG *root, const TCHAR *name, const TCHAR *str)
{
	if (inimode) {
		DWORD ret;
		ret = WritePrivateProfileString (gs (root), name, str, inipath);
		return ret;
	} else {
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegSetValueEx (rk, name, 0, REG_SZ, (CONST BYTE *)str, (_tcslen (str) + 1) * sizeof (TCHAR)) == ERROR_SUCCESS;
	}
}

int regsetint (UAEREG *root, const TCHAR *name, int val)
{
	if (inimode) {
		DWORD ret;
		TCHAR tmp[100];
		_stprintf (tmp, L"%d", val);
		ret = WritePrivateProfileString (gs (root), name, tmp, inipath);
		return ret;
	} else {
		DWORD v = val;
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegSetValueEx(rk, name, 0, REG_DWORD, (CONST BYTE*)&v, sizeof (DWORD)) == ERROR_SUCCESS;
	}
}

int regqueryint (UAEREG *root, const TCHAR *name, int *val)
{
	if (inimode) {
		int ret = 0;
		TCHAR tmp[100];
		GetPrivateProfileString (gs (root), name, PUPPA, tmp, sizeof (tmp) / sizeof (TCHAR), inipath);
		if (_tcscmp (tmp, PUPPA)) {
			*val = _tstol (tmp);
			ret = 1;
		}
		return ret;
	} else {
		DWORD dwType = REG_DWORD;
		DWORD size = sizeof (int);
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegQueryValueEx (rk, name, 0, &dwType, (LPBYTE)val, &size) == ERROR_SUCCESS;
	}
}

int regquerystr (UAEREG *root, const TCHAR *name, TCHAR *str, int *size)
{
	if (inimode) {
		int ret = 0;
		TCHAR *tmp = xmalloc (TCHAR, (*size) + 1);
		GetPrivateProfileString (gs (root), name, PUPPA, tmp, *size, inipath);
		if (_tcscmp (tmp, PUPPA)) {
			_tcscpy (str, tmp);
			ret = 1;
		}
		xfree (tmp);
		return ret;
	} else {
		DWORD size2 = *size;
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		int v = RegQueryValueEx (rk, name, 0, NULL, (LPBYTE)str, &size2) == ERROR_SUCCESS;
		*size = size2;
		return v;
	}
}

int regenumstr (UAEREG *root, int idx, TCHAR *name, int *nsize, TCHAR *str, int *size)
{
	name[0] = 0;
	str[0] = 0;
	if (inimode) {
		int ret = 0;
		int tmpsize = 65536;
		TCHAR *tmp = xmalloc (TCHAR, tmpsize);
		if (GetPrivateProfileSection (gs (root), tmp, tmpsize, inipath) > 0) {
			int i;
			TCHAR *p = tmp, *p2;
			for (i = 0; i < idx; i++) {
				if (p[0] == 0)
					break;
				p += _tcslen (p) + 1;
			}
			if (p[0]) {
				p2 = _tcschr (p, '=');
				*p2++ = 0;
				_tcscpy_s (name, *nsize, p);
				_tcscpy_s (str, *size, p2);
				ret = 1;
			}
		}
		xfree (tmp);
		return ret;
	} else {
		DWORD nsize2 = *nsize;
		DWORD size2 = *size;
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		int v = RegEnumValue (rk, idx, name, &nsize2, NULL, NULL, (LPBYTE)str, &size2) == ERROR_SUCCESS;
		*nsize = nsize2;
		*size = size2;
		return v;
	}
}

int regquerydatasize (UAEREG *root, const TCHAR *name, int *size)
{
	if (inimode) {
		int ret = 0;
		int csize = 65536;
		TCHAR *tmp = xmalloc (TCHAR, csize);
		if (regquerystr (root, name, tmp, &csize)) {
			*size = _tcslen (tmp) / 2;
			ret = 1;
		}
		xfree (tmp);
		return ret;
	} else {
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		DWORD size2 = *size;
		int v = RegQueryValueEx(rk, name, 0, NULL, NULL, &size2) == ERROR_SUCCESS;
		*size = size2;
		return v;
	}
}

int regsetdata (UAEREG *root, const TCHAR *name, const void *str, int size)
{
	if (inimode) {
		uae_u8 *in = (uae_u8*)str;
		DWORD ret;
		int i;
		TCHAR *tmp = xmalloc (TCHAR, size * 2 + 1);
		for (i = 0; i < size; i++)
			_stprintf (tmp + i * 2, L"%02X", in[i]); 
		ret = WritePrivateProfileString (gs (root), name, tmp, inipath);
		xfree (tmp);
		return ret;
	} else {
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegSetValueEx(rk, name, 0, REG_BINARY, (BYTE*)str, size) == ERROR_SUCCESS;
	}
}
int regquerydata (UAEREG *root, const TCHAR *name, void *str, int *size)
{
	if (inimode) {
		int csize = (*size) * 2 + 1;
		int i, j;
		int ret = 0;
		TCHAR *tmp = xmalloc (TCHAR, csize);
		uae_u8 *out = (uae_u8*)str;

		if (!regquerystr (root, name, tmp, &csize))
			goto err;
		j = 0;
		for (i = 0; i < _tcslen (tmp); i += 2) {
			TCHAR c1 = toupper(tmp[i + 0]);
			TCHAR c2 = toupper(tmp[i + 1]);
			if (c1 >= 'A')
				c1 -= 'A' - 10;
			else if (c1 >= '0')
				c1 -= '0';
			if (c1 > 15)
				goto err;
			if (c2 >= 'A')
				c2 -= 'A' - 10;
			else if (c2 >= '0')
				c2 -= '0';
			if (c2 > 15)
				goto err;
			out[j++] = c1 * 16 + c2;
		}
		ret = 1;
err:
		xfree (tmp);
		return ret;
	} else {
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		DWORD size2 = *size;
		int v = RegQueryValueEx(rk, name, 0, NULL, (LPBYTE)str, &size2) == ERROR_SUCCESS;
		*size = size2;
		return v;
	}
}

int regdelete (UAEREG *root, const TCHAR *name)
{
	if (inimode) {
		WritePrivateProfileString (gs (root), name, NULL, inipath);
		return 1;
	} else {
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegDeleteValue (rk, name) == ERROR_SUCCESS;
	}
}

int regexists (UAEREG *root, const TCHAR *name)
{
	if (inimode) {
		int ret = 1;
		TCHAR *tmp = xmalloc (TCHAR, _tcslen (PUPPA) + 1);
		int size = _tcslen (PUPPA) + 1;
		GetPrivateProfileString (gs (root), name, PUPPA, tmp, size, inipath);
		if (!_tcscmp (tmp, PUPPA))
			ret = 0;
		xfree (tmp);
		return ret;
	} else {
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		return RegQueryValueEx(rk, name, 0, NULL, NULL, NULL) == ERROR_SUCCESS;
	}
}

void regdeletetree (UAEREG *root, const TCHAR *name)
{
	if (inimode) {
		TCHAR *s = gsn (root, name);
		if (!s)
			return;
		WritePrivateProfileSection (s, L"", inipath);
		xfree (s);
	} else {
		HKEY rk = gr (root);
		if (!rk)
			return;
		SHDeleteKey (rk, name);
	}
}

int regexiststree (UAEREG *root, const TCHAR *name)
{
	if (inimode) {
		int ret = 0;
		int tmpsize = 65536;
		TCHAR *p, *tmp;
		TCHAR *s = gsn (root, name);
		if (!s)
			return 0;
		tmp = xmalloc (TCHAR, tmpsize / sizeof (TCHAR));
		tmp[0] = 0;
		GetPrivateProfileSectionNames (tmp, tmpsize, inipath);
		p = tmp;
		while (p[0]) {
			if (!_tcscmp (p, name)) {
				ret = 1;
				break;
			}
			p += _tcslen (p) + 1;
		}
		xfree (tmp);
		xfree (s);
		return ret;
	} else {
		int ret = 0;
		HKEY k = NULL;
		HKEY rk = gr (root);
		if (!rk)
			return 0;
		if (RegOpenKeyEx (rk , name, 0, KEY_READ, &k) == ERROR_SUCCESS)
			ret = 1;
		if (k)
			RegCloseKey (k);
		return ret;
	}
}


UAEREG *regcreatetree (UAEREG *root, const TCHAR *name)
{
	UAEREG *fkey;
	HKEY rkey;

	if (inimode) {
		TCHAR *ininame;
		if (!root) {
			if (!name)
				ininame = my_strdup (gs (NULL));
			else
				ininame = my_strdup (name);
		} else {
			ininame = xmalloc (TCHAR, _tcslen (root->inipath) + 1 + _tcslen (name) + 1);
			_stprintf (ininame, L"%s/%s", root->inipath, name);
		}
		fkey = xcalloc (UAEREG, 1);
		fkey->inipath = ininame;
	} else {
		DWORD err;
		HKEY rk = gr (root);
		if (!rk) {
			rk = HKEY_CURRENT_USER;
			name = L"Software\\Arabuusimiehet\\WinUAE";
		} else if (!name) {
			name = L"";
		}
		err = RegCreateKeyEx (rk, name, 0, NULL, REG_OPTION_NON_VOLATILE,
			KEY_READ | KEY_WRITE, NULL, &rkey, NULL);
		if (err != ERROR_SUCCESS)
			return 0;
		fkey = xcalloc (UAEREG, 1);
		fkey->fkey = rkey;
	}
	return fkey;
}

void regclosetree (UAEREG *key)
{
	if (!key)
		return;
	if (key->fkey)
		RegCloseKey (key->fkey);
	xfree (key->inipath);
	xfree (key);
}

static uae_u8 crcok[20] = { 0xaf,0xb7,0x36,0x15,0x05,0xca,0xe6,0x9d,0x23,0x17,0x4d,0x50,0x2b,0x5c,0xc3,0x64,0x38,0xb8,0x4e,0xfc };

int reginitializeinit (TCHAR **pppath)
{
	UAEREG *r = NULL;
	TCHAR tmp1[1000];
	uae_u8 crc[20];
	int s, v1, v2, v3;
	TCHAR path[MAX_DPATH], fpath[MAX_PATH];
	FILE *f;
	TCHAR *ppath = *pppath;

	if (!ppath) {
		int ok = 0;
		TCHAR *posn;
		path[0] = 0;
		GetFullPathName (_wpgmptr, sizeof path / sizeof (TCHAR), path, NULL);
		if (_tcslen (path) > 4 && !_tcsicmp (path + _tcslen (path) - 4, L".exe")) {
			_tcscpy (path + _tcslen (path) - 3, L"ini");
			if (GetFileAttributes (path) != INVALID_FILE_ATTRIBUTES)
				ok = 1;
		}
		if (!ok) {
			path[0] = 0;
			GetFullPathName (_wpgmptr, sizeof path / sizeof (TCHAR), path, NULL);
			if((posn = _tcsrchr (path, '\\')))
				posn[1] = 0;
			_tcscat (path, L"winuae.ini");
		}
		if (GetFileAttributes (path) == INVALID_FILE_ATTRIBUTES)
			return 0;
	} else {
		_tcscpy (path, ppath);
	}

	fpath[0] = 0;
	GetFullPathName (path, sizeof fpath / sizeof (TCHAR), fpath, NULL);
	if (_tcslen (fpath) < 5 || _tcsicmp (fpath + _tcslen (fpath) - 4, L".ini"))
		return 0;

	inimode = 1;
	inipath = my_strdup (fpath);
	if (!regexists (NULL, L"Version"))
		goto fail;
	r = regcreatetree (NULL, L"Warning");
	if (!r)
		goto fail;
	memset (tmp1, 0, sizeof tmp1);
	s = 200;
	if (!regquerystr (r, L"info1", tmp1, &s))
		goto fail;
	if (!regquerystr (r, L"info2", tmp1 + 200, &s))
		goto fail;
	get_sha1 (tmp1, sizeof tmp1, crc);
	if (memcmp (crc, crcok, sizeof crcok))
		goto fail;
	v1 = v2 = -1;
	regsetint (r, L"check", 1);
	regqueryint (r, L"check", &v1);
	regsetint (r, L"check", 3);
	regqueryint (r, L"check", &v2);
	regdelete (r, L"check");
	if (regqueryint (r, L"check", &v3))
		goto fail;
	if (v1 != 1 || v2 != 3)
		goto fail;
	regclosetree (r);
	return 1;
fail:
	regclosetree (r);
	if (GetFileAttributes (path) != INVALID_FILE_ATTRIBUTES)
		DeleteFile (path);
	if (GetFileAttributes (path) != INVALID_FILE_ATTRIBUTES)
		goto end;
	f = _tfopen (path, L"wb");
	if (f) {
		uae_u8 bom[3] = { 0xef, 0xbb, 0xbf };
		fwrite (bom, sizeof (bom), 1, f);
		fclose (f);
	}
	r = regcreatetree (NULL, L"Warning");
	if (!r)
		goto end;
	regsetstr (r, L"info1", L"This is unsupported file. Compatibility between versions is not guaranteed.");
	regsetstr (r, L"info2", L"Incompatible ini-files may be re-created from scratch!");
	regclosetree (r);
	if (*pppath == NULL)
		*pppath = my_strdup (path);
	return 1;
end:
	inimode = 0;
	xfree (inipath);
	return 0;
}

void regstatus (void)
{
	if (inimode)
		write_log (L"WARNING: Unsupported '%s' enabled\n", inipath);
}

int getregmode (void)
{
	return inimode;
}
