#include "card.h"
#include "path.h"

/**************************** public ****************************/

Card::Card()
{
	// ���֤��ǰ��λ����
	vector<int> key = {
		11, 12, 13, 14, 15,
		21, 22, 23,
		31, 32, 33, 34, 35, 36, 37,
		41, 42, 43,
		44, 45, 46,
		51, 52, 53, 54, 50,
		61, 62, 63, 64, 65,
		71, 81, 82
	};
	vector<string> value = {
		"����","���","�ӱ�","ɽ��","���ɹ�",
		"����","����","������",
		"�Ϻ�","����","�㽭","����","����","����","ɽ��",
		"����","����","����",
		"�㶫","����","����",
		"�Ĵ�","����","����","����","����",
		"����","����","�ຣ","����","�½�",
		"̨��","���","����"
	};
	for (int i = 0; i < key.size(); i++)
		mapPlace[key[i]] = value[i];
}

Card::~Card()
{
	idcard.release();
	img.release();
	map<int, string>().swap(mapPlace);
}

const int Card::identify(string path, string& txt)
{
	release();										// ʶ��ǰ�ͷ�֮ǰ���ڴ�
	txt.clear();									// ��ֵǰ���
	time = 0;										// ����ʶ��ʱ��

	imgPath = path;									// ����ͼƬ·��
	img = imread(imgPath);							// ��ͼ
	if (img.empty()) 								// ��֤�����ͼ��ǿ�
		return 1;
	idcard = img.clone();							// ����һ��ԭͼ
	resize();										// ���Ŵ�С
	img = idcard.clone();							// ����ͼ

	TickMeter _t;									// ��¼����ʱ��
	_t.reset();										// ����ʱ��
	_t.start();										// ��ʼ��ʱ

	preDeal();										// Ԥ����
	if (detect()) {									// ���
		_t.stop();
		time = _t.getTimeSec();						// ���ʶ���ʱ
		return 2;									// �����޷����
	}
	if (findNumber()) {								// �Һ���
		_t.stop();
		time = _t.getTimeSec();						// ���ʶ���ʱ
		return 3;									// �����޷��ҵ�����
	}
	if (predict() || correct()) {					// ʶ�����
		_t.stop();
		time = _t.getTimeSec();						// ���ʶ���ʱ
		return 4;									// ����ʶ�����
	}
	_t.stop();										// ֹͣ��ʱ
	time = _t.getTimeSec();							// ���ʶ���ʱ

	for (auto x : predictNumber)					// ת��Ϊ�ַ���
		txt += x == 10 ? "X" : to_string(x);
	idcard(Rect(10, 10, 380, 40)) = Scalar(80, 80, 80);// ���ƻ�ɫ���α���
	putText(idcard, txt, Point(20, 40), 1, 2, Scalar(0, 255, 0), 2);// ��������

	static int cnt = 0;								// ��̬���� ���ڱ��
	imwrite(savePath + to_string(cnt++) + ".jpg", idcard);	// ����ʶ����
	return 0;
}

void Card::show(string winName, const int x, const int y)
{
	if (idcard.empty())
		return;

	winName += (" | time: " + to_string(time) + " s");
	namedWindow(winName);
	moveWindow(winName, x, y);
	imshow(winName, idcard);

	//waitKey();										// �ȴ�����
	//destroyWindow(winName);							// ���ٴ���
	release();										// �ͷ��ڴ�
}

void Card::setPicFolderPath(const string path)
{
	imgFolderPath = path;
}

void Card::setTrainDataFolderPath(const string path)
{
	trainDataFolderPath = path;
}

void Card::setTrain(string _TRAIN)
{
	transform(_TRAIN.begin(), _TRAIN.end(), _TRAIN.begin(), ::tolower);
	TRAIN = _TRAIN == "true" ? true : false;

	if (TRAIN)										// �Ƿ�ѵ��
	{
		TickMeter trainT;
		trainT.reset();
		trainT.start();

		cout << "------- TRAIN SVM START -------" << endl;
		Mat trainImages;							// ѵ������
		vector<int> trainLabels;					// ѵ����ǩ

		svm = SVM::create();
		svm->setType(SVM::C_SVC);
		svm->setKernel(SVM::LINEAR);
		svm->setTermCriteria(TermCriteria(TermCriteria::MAX_ITER, 100, 1e-6));

		int classes[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };	// 10 ���� X
		vector<string> files;

		for (auto x : classes)						// ����ÿ�������ļ���
		{
			getFileNames(trainDataFolderPath + to_string(x), files);// ��ȡ�÷����ļ��е�����ͼƬ
			for (auto file : files)					// ��ȡ����ͼƬ
			{
				img = imread(file, 0);				// ��ȡ�Ҷ�ͼ
				if (img.empty()) continue;			// �ж��Ƿ�Ϊ��
				threshold(img, img, 10, 255, THRESH_OTSU); // ��ֵ��
				img.convertTo(img, CV_32FC1);		// ת��Ϊ float ���͵�ͨ��ͼ��
				trainImages.push_back(img.reshape(0, 1));// ͼƬת��Ϊһ�� ����ѵ��������
				trainLabels.push_back(x);			// �����ӦͼƬ��ǩ
			}
			files.clear();							// ������һ���ļ���ǰ���
		}
		img.release();								// �ͷ� img ռ�õ��ڴ�

		// ѵ��
		svm->train(trainImages, ROW_SAMPLE, trainLabels);// ѵ��ģ��
		svm->save(trainDataFolderPath + "svm.xml");	//����ģ��
		svm->clear();
		trainT.stop();
		cout << "train time: " << trainT.getTimeSec() << " s" << endl;
		cout << "------- TRAIN SVM  END  -------" << endl;
	}
}

void Card::setDebug(string _Debug)
{
	transform(_Debug.begin(), _Debug.end(), _Debug.begin(), ::tolower);
	DEBUG = _Debug == "true" ? true : false;
}

void Card::setSavePath(const string path)
{
	savePath = path;
}

const string Card::getPicFolderPath()
{
	return imgFolderPath;
}

bool Card::is_DEBUG()
{
	return this->DEBUG;
}

/**************************** private ***************************/
// ʹԭͼ�ߴ������� toRows * toCols ����
void Card::resize(const int toRows, const int toCols)
{
	int lr = 0, lc = 0;								// ��¼����ʱ��С�� row col
	int hr = idcard.rows, hc = idcard.cols;			// ��¼����ʱ�ϴ�� row col	
	int mr = (lr + hr) >> 1, mc = (lc + hc) >> 1;	// ��¼����ʱ���ֵ� row col
	int size = mr * mc;								// ��ǰ���ֵ�ߴ�
	int toSize = toRows * toCols;					// Ŀ��ߴ�
	int sub = toSize - size;						// ��ǰ�ߴ���Ŀ��ߴ��

	// ʹ�ߴ��С�����������
	int delta = (toCols + toRows) << 1;				// ����������Ŀ��ߴ�����
	while (abs(sub) > delta)						// ��������ʱ����ѭ��
	{
		// ԭͼ����Ŀ��ͼ
		if (sub < 0) { hr = mr; hc = mc; }			// ���ͽϴ�� row col
		// ԭͼС��Ŀ��ͼ
		else { hr += mr; hc += mc; lr = mr; lc = mc; }// ������С�� row col
		mr = (hr + lr) >> 1;						// ���¶��ֵ� row
		mc = (hc + lc) >> 1;						// ���¶��ֵ� col
		size = mr * mc;								// ���µ�ǰ�ߴ�
		sub = toSize - size;						// ���µ�ǰ��ֵ
	}
	cv::resize(idcard, idcard, Size(mc, mr));		// OpenCV ˫���Բ�ֵ����
}

void Card::resize(Mat& _img)
{
	int k = _img.rows - _img.cols;						// �� - ��
	if (k > 0) {										// ����� < �� �ú�ɫ������Ҳ���
		copyMakeBorder(_img, _img, 0, 0, k/2, k - k/2, BORDER_CONSTANT, 0);
	}
	else {												// �ú�ɫ������²���
		k = -k;
		copyMakeBorder(_img, _img, k/2, k - k/2, 0, 0, BORDER_CONSTANT, 0);	
	}
	cv::resize(_img, _img, Size(28, 28));				// ��һ���� 28 * 28
}

void Card::preDeal()
{
	if (DEBUG)
	{
		imshow("0_src", idcard);
	}
	bilateralFilter(idcard, img, 7, 10, 5);				// ˫���˲�
	idcard = img.clone();								// ����˫���˲����ԭͼ
	if (DEBUG)
	{
		imshow("predeal_0_bilateraFilter", img);
	}

	// �ҶȻ�
	//vector<Mat> m;										// �洢����� BGR ͨ��
	//cv::split(idcard, m);								// ͨ������
	//img = m[2].clone();									// ���� R ͨ��
	//vector<Mat>().swap(m);								// ��� m ռ�õ��ڴ�
	cvtColor(img, img, COLOR_BGR2GRAY);
	if (DEBUG)
	{
		imshow("predeal_1_gray", img);
	}

	// ��̬ѧ��Ե���
	Mat tmp = img.clone();								// ��ʱ�洢�ռ�
	morphologyEx(										// ��̬ѧ����
		img,											// ����ͼ��
		tmp,											// ���ͼ��
		MORPH_CLOSE,									// ָ�������� ��ͨ��������
		getStructuringElement(MORPH_RECT, Size(7, 7))	// ��ȡ�ṹ��
	);
	img = tmp - img;									// ����

	if (DEBUG)
	{
		imshow("predeal_2_close", tmp);
		imshow("predeal_3_gray - close", img);
	}

	tmp.release();										// �ͷ���ʱ�ռ�

	// ��ֵ��
	threshold(img, img, 0, 255, THRESH_OTSU);			// ��ֵ��
	if (DEBUG)
	{
		imshow("predeal_4_threshold_otsu", img);
	}
}

const int  Card::detect()
{
	// ������ �γ���ͨ����
	morphologyEx(										// ��̬ѧ����
		img,											// ����ͼ��
		img,											// ���ͼ��
		MORPH_CLOSE,									// ָ�������� ��ͨ��������
		getStructuringElement(MORPH_RECT, Size(21, 13))	// ��ȡ�ṹ��
	);

	if (DEBUG)
	{
		imshow("detect_0_close", img);
	}

	// ��������
	vector<vector<Point>> contours;						// ����������
	vector<Vec4i> hierarchy;							// �����Ĳ㼶��ϵ
	findContours(										// ��������
		img,											// findContours ��ı�����ͼ��
		contours,										// �������
		hierarchy,										// �㼶��ϵ �˴�ֻ��һ��ռλ
		RETR_TREE,										// ����ʽ����
		CHAIN_APPROX_NONE								// ��������
	);	
	img.release();										// ��� img ռ�õ��ڴ�
	vector<Vec4i>().swap(hierarchy);					// ��� hierarchy ռ�õ��ڴ�

	if (DEBUG)
	{
		Mat dbg_img = Mat::zeros(idcard.size(), CV_8UC1);
		for (int dbg_i = 0; dbg_i < contours.size(); dbg_i++)
			drawContours(dbg_img, contours, dbg_i, 255, 1, 8);
		imshow("detect_1_find_contours", dbg_img);
	}

	// ɸѡ����
	vector<vector<Point>> contours_number;				// ������ܵĺ�������
	for (auto itr = contours.begin(); itr != contours.end(); itr++)	
		if (itr->size() > 400)							// ���������������� 400 ��
			contours_number.push_back(*itr);			// ���������
	vector<vector<Point>>().swap(contours);				// �ͷ� contours ռ�õ��ڴ�

	if (DEBUG)
	{
		Mat dbg_img = Mat::zeros(idcard.size(), CV_8UC1);
		for (int dbg_i = 0; dbg_i < contours_number.size(); dbg_i++)
			drawContours(dbg_img, contours_number, dbg_i, 255, 1, 8);
		imshow("detect_2_filter_contours", dbg_img);
	}

	// ��С��Χ���ο�
	int i = 0;
	for (auto itr = contours_number.begin(); itr != contours_number.end(); itr++, i++)
	{
		RotatedRect rotatedRect = minAreaRect(*itr);	// �ӵ㼯�����С��Χ��ת����
		const float width = rotatedRect.size.width;		// x ����ʱ����ת�õ��ĵ�һ���߶���Ϊ��
		const float height = rotatedRect.size.height;	// ��һ���߶���Ϊ��
		const float k = height / width;					// �߱ȿ�
		if (											// ɸѡ
			width < 15 || height < 15					// �߳���С
			|| (0.1 < k && k < 10)						// ��߱���һ����Χ�����Ǻܳ��ľ��Σ�
		)
			continue;

		rotatedRects.push_back(rotatedRect);			// ����
	}
	vector<vector<Point>>().swap(contours_number);		// �ͷ� contours_number ռ�õ��ڴ�

	if (rotatedRects.empty()) return 2;					// ���δ��⵽������������ͨ��ֱ�ӷ��� 2

	if (DEBUG)
	{
		Point2f dbg_p[4];
		Mat dbg_img = idcard.clone();
		for (auto dbg_rotatedRect : rotatedRects)
		{
			dbg_rotatedRect.points(dbg_p);
			for (int dbg_i = 0; dbg_i < 4; dbg_i++)
				line(dbg_img, dbg_p[dbg_i], dbg_p[(dbg_i + 1) % 4], Scalar(0, 0, 255), 2, 8);
		}
		imshow("detect_3_filter_rotated_rect", dbg_img);
	}

	return 0;
}

const int Card::findNumber()
{
	// ��¼������������ת���ΰ����������� x ���д��ҵ�������
	sort(rotatedRects.begin(), rotatedRects.end(), [](RotatedRect a, RotatedRect b) {
		return a.center.x > b.center.x;
	});

	// ͸�ӱ任����
	const int toCols = 504, toRows = 28;				// ����ϣ���ĺ����ͼ��С
	const int offset = 7;								// ��س���һȦ
	vector<vector<Point>> contours;						// ������������

	// ����ÿ����ת���� �Ƿ��Ǻ�������
	for (auto itr = rotatedRects.begin(); itr != rotatedRects.end(); itr++)
	{
		// ������ת���ζ���
		Point2f p[4];
		itr->points(p);

		// �� p �����������
		sort(p, p + 4, [](Point2f a, Point2f b) {			// ���� x ��С��������
			return a.x < b.x;
		});
		if (p[0].y > p[1].y) swap(p[0], p[1]);
		if (p[2].y < p[3].y) swap(p[2], p[3]);
		if (abs(p[0].y - p[1].y) > 60)						// ��Ҫ��� 
			return 3;

		// �Ը���ת����͸�ӱ任
		Point2f pSrc[4] = {									// ͸�ӱ任 4 ��Դ��
			Point2f(p[0].x - offset, p[0].y - offset), Point2f(p[3].x + offset, p[3].y - offset),
			Point2f(p[1].x - offset, p[1].y + offset), Point2f(p[2].x + offset, p[2].y + offset)
		};
		Point2f pDst[4] = {									// ͸�ӱ任 4 ��Ŀ���
			Point2f(0, 0),		Point2f(toCols, 0),
			Point2f(0, toRows),	Point2f(toCols, toRows)
		};
		warpPerspective(									// ͸�ӱ任����
			idcard,											// ����ͼ��
			img,											// ���ͼ��
			getPerspectiveTransform(pSrc, pDst),			// ��͸�ӱ任����
			Size(toCols, toRows)							// ���ͼ���С
		);

		// �ж��Ƿ����18�����������е��ַ�����ͨ��
		cvtColor(img, img, COLOR_BGR2GRAY);					// �ҶȻ�
		idcardNumber = img.clone();							// ������ܵ����� ����ʶ��

		// ���� ��Ե���
		Mat tmp = img.clone();								// ��ʱ�洢�ռ�
		morphologyEx(										// ��̬ѧ����
			img,											// ����ͼ��
			tmp,											// ���ͼ��
			MORPH_CLOSE,									// ָ�������� ��ͨ��������
			getStructuringElement(MORPH_RECT, Size(7, 7))	// ��ȡ 7 * 7�ṹ��
		);
		img = tmp - img;									// ����

		blur(img, img, Size(3, 3));							// 3 * 3 ��ֵ�˲�
		threshold(img, img, 0, 255, THRESH_OTSU);			// otsu ��ֵ��

		// ������ ��ͨ���ѵıʻ�
		morphologyEx(										// ��̬ѧ����
			img,											// ����ͼ��
			img,											// ���ͼ��
			MORPH_CLOSE,									// ָ�������� ��ͨ��������
			getStructuringElement(MORPH_RECT, Size(3, 7))	// ��ȡ�ṹ��
		);

		findContours(img, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);	// �����������

		if (contours.size() == 18)							// �������� 18 ��˵���Ǻ�������
		{
			// �� idcard �Ͽ����������
			for (int i = 0; i < 4; i++)
				line(idcard, p[i], p[(i + 1) % 4], Scalar(0, 255, 0), 2, 8);

			if (DEBUG)
			{
				imshow("find_number_0_number_rotated_rect", idcard);
			}
			break;
		}
	}
	vector<RotatedRect>().swap(rotatedRects);				// ��� rotatedRects �ڴ�
	if (contours.size() != 18)								// ������ͨ����ַ����������� �򷵻��޷��ҵ���������
	{
		idcardNumber.release();
		return 3;
	}

	// ������������ ����������
	sort(contours.begin(), contours.end(), [](vector<Point> a, vector<Point> b) {
		return boundingRect(a).br().x < boundingRect(b).br().x;
	});

	// ���������α���ÿ������
	vector<Mat> mv;
	for (auto itr = contours.begin(); itr != contours.end(); itr++)
	{
		Rect rect = boundingRect(*itr);						// ���Χ����
		Mat tmp = idcardNumber(Rect(rect)).clone();			// ��ȡ��������
		threshold(tmp, tmp, 0, 255, THRESH_OTSU | THRESH_BINARY_INV); // OTSU ��ֵ�� 
		resize(tmp);										// �ߴ��һ�� 28 * 28
		idcardNumbers.push_back(tmp);						// ���������

		//static int cnt = 0;									// ��������ѵ�� �����²����ĵ�������ͼƬ
		//imwrite("E:/������/idcard/x64/Debug/data/trainData/" + to_string(cnt++) + ".jpg", tmp);
	}

	if (DEBUG)
	{
		Mat dbg_img = Mat::zeros(img.size(), CV_8UC3);
		int dbg_b = 60, dbg_g = 20, dbg_r = 100;
		Scalar dbg_color(dbg_b, dbg_g, dbg_r);
		for (int dbg_i = 0; dbg_i < contours.size(); dbg_i++)
		{
			drawContours(dbg_img, contours, dbg_i, dbg_color, 1, 8);
			dbg_b = (dbg_b + 20) % 256;
			dbg_g = (dbg_g + 40) % 256;
			dbg_r = (dbg_r + 80) % 256;
			dbg_color = Scalar(dbg_b, dbg_g, dbg_r);
		}
		Mat dbg_dst = Mat::zeros(Size(img.cols, img.rows * 3), CV_8UC3);
		merge(vector<Mat>({ idcardNumber, idcardNumber, idcardNumber }), idcardNumber);
		idcardNumber.copyTo(dbg_dst(Rect(0, 0, img.cols, img.rows)));
		dbg_img.copyTo(dbg_dst(Rect(0, img.rows, img.cols, img.rows)));

		dbg_img = Mat::zeros(Size(32 * 18, 28), CV_8UC1);
		for (int dbg_i = 0; dbg_i < idcardNumbers.size(); dbg_i++)
			idcardNumbers[dbg_i].copyTo(dbg_img(Rect(dbg_i * 32, 0, 28, 28)));
		cv::resize(dbg_img, dbg_img, Size(img.cols, img.rows));

		merge(vector<Mat>({ dbg_img, dbg_img, dbg_img }), dbg_img);
		dbg_img.copyTo(dbg_dst(Rect(0, img.rows * 2, img.cols, img.rows)));

		imshow("find_number_1_single_number", dbg_dst);
		dbg_dst.release();
		dbg_img.release();
		waitKey(1);
	}

	idcardNumber.release();									// �ͷ� idcardNumber ռ�õ��ڴ�
	return 0;
}

const int Card::predict()
{
	svm = SVM::load(trainDataFolderPath + "svm.xml");// ��ȡģ��

	// ���ʶ��
	for (auto itr = idcardNumbers.begin(); itr != idcardNumbers.end(); itr++)
	{
		itr->convertTo(img, CV_32FC1);				// ת��Ϊ float ���͵�ͨ��ͼƬ
		float res = svm->predict(img.reshape(0, 1));// Ԥ����
		predictNumber.push_back(res);				// ����Ԥ����
	}

	img.release();
	svm->clear();

	return correct() ? 1 : 0;
}

const int Card::correct()
{
	int sum = 0;
	vector<int> W = { 7, 9, 10, 5, 8, 4, 2, 1, 6, 3, 7, 9, 10, 5, 8, 4, 2, 1 };	// Ȩ��
	for (int i = 0; i < W.size(); i++)							// ��Ȩ��
		sum += predictNumber[i] * W[i];
	sum %= 11;													// ȡ�� 11

	if (
		sum == 1												// �������Ϊ 1
		|| (sum == 10 && predictNumber.back() == 10)			// ���� (10 �����һλ X)
		|| mapPlace.find(predictNumber[0]*10 + predictNumber[1]) != mapPlace.end() // ���ǰ��λ��ȷ
	)	
		return 0;
	else
		return 1;
}

void Card::release()
{
	idcard.release();
	img.release();
	idcardNumber.release();
	imgPath.clear();
	vector<RotatedRect>().swap(rotatedRects);
	vector<int>().swap(predictNumber);
	vector<Mat>().swap(idcardNumbers);
}
