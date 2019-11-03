#include "card.h"
#include "path.h"

int main(int argc, char** argv)
{
	Card card;												// ʶ�����֤������

	if (argc == 1)											// ���û�в��� ��Ĭ�����·��
	{
		card.setPicFolderPath("./data/pic");				// ͼƬ��Ŀ¼ ע�����û��б�� '/'
		card.setTrainDataFolderPath("./data/trainData/");	// ѵ������Ŀ¼ ע�������б�� '/'
		card.setSavePath("./data/res/");					// ���ý������·��
		card.setTrain("FALSE");								// ���� SVM �Ƿ�����ѵ�� TRUE / FALSE
		card.setDebug("FALse");								// �����Ƿ� DEBUG ģʽ TRUE / FALSE
	}
	else if (argc == 6)
	{
		/// params 1: ��ʶ��ͼƬ�ļ���·��
		/// params 2: SVM ѵ������·��
		/// params 3: ���ý������·��
		/// params 4: �Ƿ�ѵ�� SVM
		/// params 5: ���� debug ģʽ
		card.setPicFolderPath(argv[1]);						// ��Ҫ������
		card.setTrainDataFolderPath(argv[2]);				// ��Ҫ������
		card.setSavePath(argv[3]);							// ���ý������·��
		card.setTrain(argv[4]);								// TRUE / FALSE
		card.setDebug(argv[5]);								// TRUE / FALSE
	}
	else
	{
		cout << "please check your dir" << endl;
		system("pause");
		return 0;
	}

	TickMeter totalT;										// ��¼��ʱ��
	totalT.reset();											// ����

	string txt;												// ����ʶ����
	vector<string> files;									// ͼƬ·���б�
	getFileNames(card.getPicFolderPath(), files);			// ���·���б�
	cout << "------- IDENTIFY START -------" << endl;
	for (auto file : files)									// ����
	{
		totalT.start();										// ��ʼ��ʱ
		int res = card.identify(file, txt);					// ʶ��
		if (res == 0) {
			cout << "| result: " + txt + " | " + file << endl; // �������
			if (card.is_DEBUG())
			{
				card.show(file);								// ��ʾ������ʶ����ĺ��� ���� show ����ͷ��ڴ�
			}
		}
		else if (res == 1)
			cout << "| error : no such picture | " + file << endl; // ��ʾͼƬΪ��
		else if (res == 2)
			cout << "| error : can not detect | " + file << endl; // ��ʾ���ܼ�⵽����
		else if (res == 3)
			cout << "| error : can not find number area | " + file << endl; // ���벻��18λ
		else if (res == 4)
			cout << "| error : wrong number | " + file << endl; // ��ʾʶ�����

		totalT.stop();										// ��ͣ��ʱ
		if (card.is_DEBUG())
		{
			waitKey();											// �ȴ�����
			destroyAllWindows();								// �������д���
		}
	}

	totalT.stop();											// ֹͣ��ʱ
	cout << "total time: " << totalT.getTimeSec() << " s" << endl;	// ��ʱ��
	cout << "------- IDENTIFY  END -------" << endl;

	system("pause");
	return 0;
}

