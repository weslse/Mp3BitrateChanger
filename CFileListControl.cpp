#include "pch.h"
#include "CFileListControl.h"
BEGIN_MESSAGE_MAP(CFileListControl, CListCtrl)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()




void CFileListControl::OnDropFiles(HDROP hDropInfo)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	int nFiles;
	char szPathName[MAX_PATH];
	CString strFileName;

	//��ӵ� ������ ����
	nFiles = ::DragQueryFile(hDropInfo, 0xFFFFFFFF, (LPWSTR)szPathName, MAX_PATH);

	// ��ӵ� ������ ������ŭ ������ ���鼭 ���� ��θ� �޽��� �ڽ��� ���
	for (int i = nFiles - 1; i >= 0; i--)
	{
		// ������ ��� ����
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
