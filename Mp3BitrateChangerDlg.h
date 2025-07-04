﻿
// Mp3BitrateChangerDlg.h: 헤더 파일
//

#pragma once
#include "CFileListControl.h"
#include <filesystem>

// CMp3BitrateChangerDlg 대화 상자
class CMp3BitrateChangerDlg : public CDialogEx
{
	// 생성입니다.
public:
	CMp3BitrateChangerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MP3BITRATECHANGER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


	// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	std::filesystem::path make_temp_ascii_path(int index);

public:
	CFileListControl m_ListCtrl;
	afx_msg void OnBnClickedConvert();
	afx_msg void OnBnClickedDel();
	LRESULT OnUpdateProgress(WPARAM wParam, LPARAM lParam);
	CProgressCtrl convert_progress_bar;
};
