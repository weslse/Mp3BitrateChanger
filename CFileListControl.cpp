#include "pch.h"
#include "CFileListControl.h"
BEGIN_MESSAGE_MAP(CFileListControl, CListCtrl)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()




void CFileListControl::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	int nFiles;
	char szPathName[MAX_PATH];
	CString strFileName;

	//드롭된 파일의 갯수
	nFiles = ::DragQueryFile(hDropInfo, 0xFFFFFFFF, (LPWSTR)szPathName, MAX_PATH);

	// 드롭된 파일의 갯수만큼 루프를 돌면서 파일 경로를 메시지 박스로 출력
	for (int i = nFiles - 1; i >= 0; i--)
	{
		// 파일의 경로 얻어옴
		::DragQueryFile(hDropInfo, i, (LPWSTR)szPathName, MAX_PATH);
		//AfxMessageBox((LPCTSTR)szPathName);

		int num = GetItemCount();
		CString str_num;
		str_num.Format(_T("%d"), num);
		InsertItem(num, str_num);
		SetItem(num, 0, LVIF_TEXT, (LPCTSTR)szPathName, NULL, NULL, NULL, NULL);
	}

	::DragFinish(hDropInfo);

	CListCtrl::OnDropFiles(hDropInfo);
}
