// Mp3BitrateChangerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "Mp3BitrateChanger.h"
#include "Mp3BitrateChangerDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <iostream>
#include <string>
#include <future>
#include <thread>
#include <vector>
#include <shellapi.h> // 파일 상단에 추가
#include <mutex> // 추가

#include "audiorw/audiorw.hpp"

#define WM_UPDATE_PROGRESS (WM_USER + 1)

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
	DDX_Control(pDX, IDC_PROGRESS_CONVERT, convert_progress_bar);
}

BEGIN_MESSAGE_MAP(CMp3BitrateChangerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_CONVERT, &CMp3BitrateChangerDlg::OnBnClickedConvert)
	ON_BN_CLICKED(IDDEL, &CMp3BitrateChangerDlg::OnBnClickedDel)
	ON_MESSAGE(WM_UPDATE_PROGRESS, &CMp3BitrateChangerDlg::OnUpdateProgress)
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

	m_ListCtrl.InsertColumn(0, (LPCTSTR)(CString)("파일 경로"), LVCFMT_LEFT, rt.Width());

	convert_progress_bar.SetRange(0, 100);

	SetWindowText(_T("Mp3BitrateChanger"));

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
	// 변환 시작 시 프로그레스바 초기화
	convert_progress_bar.SetPos(0);

	constexpr int MAX_LIMIT_BITE = 20971520; // 20MB
	TCHAR szPath[MAX_PATH] = { 0, };
	SHGetSpecialFolderPath(NULL, szPath, CSIDL_PERSONAL, FALSE);

	std::filesystem::path output_path;
	output_path = std::filesystem::path(szPath) / L"Mp3BitrateChanger";
	if (!std::filesystem::is_directory(output_path))
		std::filesystem::create_directory(output_path);

	int total = m_ListCtrl.GetItemCount();

	for (int i = 0; i < total; i++)
	{
		try {
			CString item = m_ListCtrl.GetItemText(i, 0);
			std::wstring wpath(item);
			std::filesystem::path full_path(wpath);
			int size = 0;
			if (!std::filesystem::exists(full_path)) {
				CString msg;
				msg.Format(_T("파일이 존재하지 않습니다:\n%s"), item);
				AfxMessageBox(msg);
				continue;
			}
			size = std::filesystem::file_size(full_path);

			int bitrate_idx = 0;
			if (size < MAX_LIMIT_BITE)
				continue;

			std::filesystem::path input_path = full_path.parent_path();
			std::wstring input_file_name = full_path.filename().wstring();
			std::wstring replace_name = L"foo" + std::to_wstring(i + 1) + L".mp3";
			std::filesystem::path temp_path = input_path / replace_name;
			int output_size = 0;

			// 임시 영어 경로 생성
			std::filesystem::path ascii_temp_path = make_temp_ascii_path(i);

			// 한글 경로를 임시 영어 경로로 복사
			std::filesystem::copy_file(full_path, ascii_temp_path, std::filesystem::copy_options::overwrite_existing);

			int loop_count = 0;
			const int MAX_LOOP = 10;
			do
			{
				std::string utf8_path = wstring_to_utf8(ascii_temp_path.wstring());
				double sample_rate;
				std::vector<std::vector<double>> audio =
					audiorw::read(utf8_path, sample_rate);

				auto output_temp_filename = (output_path / replace_name).wstring();
				audiorw::write(audio, output_temp_filename, sample_rate, bitrate_idx++);

				auto output_filename = (output_path / input_file_name).wstring();
				std::filesystem::rename(output_temp_filename, output_filename);

				output_size = std::filesystem::file_size(output_filename);

				loop_count++;
				

				if (loop_count > MAX_LOOP) {
					AfxMessageBox(_T("파일 크기를 제한 이하로 줄일 수 없습니다. 변환을 중단합니다."));
					break;
				}
			} while (output_size > MAX_LIMIT_BITE);

			// 임시 파일 삭제
			std::filesystem::remove(ascii_temp_path);

			int percent = static_cast<int>((i + 1) * 100 / total);
			OnUpdateProgress(percent, 0);
		}
		catch (const std::filesystem::filesystem_error& e) {
			CString msg;
			msg.Format(_T("파일 시스템 예외 발생:\n%s"), CString(e.what()));
			AfxMessageBox(msg);
		}
		catch (const std::exception& e) {
			CString msg;
			msg.Format(_T("예외 발생:\n%s"), CString(e.what()));
			AfxMessageBox(msg);
		}
		catch (...) {
			AfxMessageBox(_T("알 수 없는 예외가 발생했습니다."));
		}
	}
	OnUpdateProgress(100, 0);
	AfxMessageBox(_T("Done!!"));

	ShellExecute(
		NULL,
		_T("open"),
		output_path.c_str(),
		NULL,
		NULL,
		SW_SHOWNORMAL
	);
}

void CMp3BitrateChangerDlg::OnBnClickedDel()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	POSITION pos;
	pos = m_ListCtrl.GetFirstSelectedItemPosition();
	int idx = m_ListCtrl.GetNextSelectedItem(pos);
	m_ListCtrl.DeleteItem(idx);
}

LRESULT CMp3BitrateChangerDlg::OnUpdateProgress(WPARAM wParam, LPARAM lParam)
{
	int percent = static_cast<int>(wParam);
	convert_progress_bar.SetPos(percent);
	return 0;
}

std::filesystem::path CMp3BitrateChangerDlg::make_temp_ascii_path(int index) {
    wchar_t tempPath[MAX_PATH] = L"C:\Temp";
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring tempFileName = L"mp3_temp_" + std::to_wstring(index) + L".mp3";
    return std::filesystem::path(tempPath) / tempFileName;
}
