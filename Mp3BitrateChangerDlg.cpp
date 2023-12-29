﻿
// Mp3BitrateChangerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "Mp3BitrateChanger.h"
#include "Mp3BitrateChangerDlg.h"
#include "afxdialogex.h"

#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <iostream>
#include "audiorw/audiorw.hpp"

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMp3BitrateChangerDlg 대화 상자



CMp3BitrateChangerDlg::CMp3BitrateChangerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MP3BITRATECHANGER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMp3BitrateChangerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDLISTCTRL, m_ListCtrl);
}

BEGIN_MESSAGE_MAP(CMp3BitrateChangerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_CONVERT, &CMp3BitrateChangerDlg::OnBnClickedConvert)
	ON_BN_CLICKED(IDDEL, &CMp3BitrateChangerDlg::OnBnClickedDel)
END_MESSAGE_MAP()


// CMp3BitrateChangerDlg 메시지 처리기

BOOL CMp3BitrateChangerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	// List Control 초기화
	CRect rt;
	m_ListCtrl.GetWindowRect(&rt);
	m_ListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT); // 리스트 컨트롤에 선표시 및 Item 선택시 한행 전체 선택

	m_ListCtrl.InsertColumn(0, (LPCTSTR)(CString)("내용"), LVCFMT_LEFT, rt.Width());

	ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);

	DragAcceptFiles();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CMp3BitrateChangerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CMp3BitrateChangerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CMp3BitrateChangerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMp3BitrateChangerDlg::OnBnClickedConvert()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

		// Read the file
	std::cout << "Reading from file " << ".\\test.mp3" << std::endl;
	double sample_rate;
	std::vector<std::vector<double>> audio =
		audiorw::read(".\\test.mp3", sample_rate);

	// Write the file
	std::cout << "Writing to file " << ".\\out.mp3" << std::endl;
	audiorw::write(audio, ".\\out.mp3", sample_rate);
}


void CMp3BitrateChangerDlg::OnBnClickedDel()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	POSITION pos;
	pos = m_ListCtrl.GetFirstSelectedItemPosition();
	int idx = m_ListCtrl.GetNextSelectedItem(pos);
	m_ListCtrl.DeleteItem(idx);
}
