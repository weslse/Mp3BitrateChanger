#pragma once
#include <afxcmn.h>

class CFileListControl :
    public CListCtrl
{
public:
    DECLARE_MESSAGE_MAP()
        afx_msg void OnDropFiles(HDROP hDropInfo);
};

