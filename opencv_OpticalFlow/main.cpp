#include <Windows.h>
#include <opencv2\opencv.hpp>

enum{
OPT_X_STEP = 5,
OPT_Y_STEP = 5,
HS_LAMBDA = 100,
LK_SIZE = 15,
//BMは時間が掛かるので動画で使うにはチューニングが必要
//マッチングテンプレートの大きさ
BM_BLOCK = 10,
//マッチング時の間引き量(1だと間引き無し)
BM_SHIFT = 3,
//マッチングの探索範囲
BM_MAX = 50,
//PyrLK用
PYRLK_COUNT = 150
};

void main()
{
int x,y;
IplImage* currImg = NULL;
IplImage* currBImg = NULL;
IplImage* currGImg = NULL;
IplImage* currRImg = NULL;
IplImage* prevBImg = NULL;
IplImage* prevGImg = NULL;
IplImage* prevRImg = NULL;
IplImage* opImg = NULL;
CvCapture* cap = cvCreateCameraCapture(0);
if (cap == NULL){
    return;
}
//USBカメラの始動待ち(Windows)
Sleep(100);
currImg = cvQueryFrame(cap);
//オプティカルフローの算出は1ch画像が必要
//ループのためにダミーフレームを取得しておく
currGImg = cvCreateImage(cvGetSize(currImg),currImg->depth, 1);
currBImg = cvCloneImage(currGImg);
currRImg = cvCloneImage(currGImg);
prevBImg = cvCloneImage(currGImg);
prevGImg = cvCloneImage(currGImg);
prevRImg = cvCloneImage(currGImg);
//B,G,Rに分離
cvSplit(currImg,currBImg,currGImg,currRImg,NULL);
opImg = cvCloneImage(currImg);
//各色用のベクトル置場
CvMat* vxB = cvCreateMat(currImg->height, currImg->width, CV_32FC1);
CvMat* vyB = cvCreateMat(currImg->height, currImg->width, CV_32FC1);
CvMat* vxG = cvCreateMat(currImg->height, currImg->width, CV_32FC1);
CvMat* vyG = cvCreateMat(currImg->height, currImg->width, CV_32FC1);
CvMat* vxR = cvCreateMat(currImg->height, currImg->width, CV_32FC1);
CvMat* vyR = cvCreateMat(currImg->height, currImg->width, CV_32FC1);
cvSetZero(vxB);
cvSetZero(vyB);
cvSetZero(vxG);
cvSetZero(vyG);
cvSetZero(vxR);
cvSetZero(vyR);
//BM用の置場
//ブロックサイズで割った大きさになる
CvMat* vxB_BM = cvCreateMat(int(ceil(double(currImg->height) / BM_BLOCK)), int(ceil(double(currImg->width) / BM_BLOCK)), CV_32FC1);
CvMat* vyB_BM = cvCreateMat(int(ceil(double(currImg->height) / BM_BLOCK)), int(ceil(double(currImg->width) / BM_BLOCK)), CV_32FC1);
CvMat* vxG_BM = cvCreateMat(int(ceil(double(currImg->height) / BM_BLOCK)), int(ceil(double(currImg->width) / BM_BLOCK)), CV_32FC1);
CvMat* vyG_BM = cvCreateMat(int(ceil(double(currImg->height) / BM_BLOCK)), int(ceil(double(currImg->width) / BM_BLOCK)), CV_32FC1);
CvMat* vxR_BM = cvCreateMat(int(ceil(double(currImg->height) / BM_BLOCK)), int(ceil(double(currImg->width) / BM_BLOCK)), CV_32FC1);
CvMat* vyR_BM = cvCreateMat(int(ceil(double(currImg->height) / BM_BLOCK)), int(ceil(double(currImg->width) / BM_BLOCK)), CV_32FC1);
//PyrLK用の画像と置場置場
int corner_countB = PYRLK_COUNT;
int corner_countG = PYRLK_COUNT;
int corner_countR = PYRLK_COUNT;
IplImage* eig_imageB = cvCreateImage(cvGetSize(currImg),IPL_DEPTH_32F,1);
IplImage* temp_imageB = cvCloneImage(eig_imageB);
IplImage* eig_imageG = cvCreateImage(cvGetSize(currImg),IPL_DEPTH_32F,1);
IplImage* temp_imageG = cvCloneImage(eig_imageG);
IplImage* eig_imageR = cvCreateImage(cvGetSize(currImg),IPL_DEPTH_32F,1);
IplImage* temp_imageR = cvCloneImage(eig_imageR);
CvPoint2D32f* corners1B = (CvPoint2D32f *) cvAlloc (corner_countB * sizeof (CvPoint2D32f));
CvPoint2D32f* corners2B = (CvPoint2D32f *) cvAlloc (corner_countB * sizeof (CvPoint2D32f));
CvPoint2D32f* corners1G = (CvPoint2D32f *) cvAlloc (corner_countG * sizeof (CvPoint2D32f));
CvPoint2D32f* corners2G = (CvPoint2D32f *) cvAlloc (corner_countG * sizeof (CvPoint2D32f));
CvPoint2D32f* corners1R = (CvPoint2D32f *) cvAlloc (corner_countR * sizeof (CvPoint2D32f));
CvPoint2D32f* corners2R = (CvPoint2D32f *) cvAlloc (corner_countR * sizeof (CvPoint2D32f));
IplImage* prev_pyramidB = cvCreateImage (cvSize (currImg->width + 8, currImg->height / 3), IPL_DEPTH_8U, 1);
IplImage* curr_pyramidB = cvCreateImage (cvSize (currImg->width + 8, currImg->height / 3), IPL_DEPTH_8U, 1);
IplImage* prev_pyramidG = cvCreateImage (cvSize (currImg->width + 8, currImg->height / 3), IPL_DEPTH_8U, 1);
IplImage* curr_pyramidG = cvCreateImage (cvSize (currImg->width + 8, currImg->height / 3), IPL_DEPTH_8U, 1);
IplImage* prev_pyramidR = cvCreateImage (cvSize (currImg->width + 8, currImg->height / 3), IPL_DEPTH_8U, 1);
IplImage* curr_pyramidR = cvCreateImage (cvSize (currImg->width + 8, currImg->height / 3), IPL_DEPTH_8U, 1);
char* statusB = (char *) cvAlloc (corner_countB);
char* statusG = (char *) cvAlloc (corner_countG);
char* statusR = (char *) cvAlloc (corner_countR);
//止め時は試行錯誤が必要。
//CV_TERMCRIT_ITERは試行回数で制限
//CV_TERMCRIT_EPSは誤差で制限
//オプティカルフローを厳密に求める場合は後者を
//速く求める場合は前者を、中間を取りたければORで指定し両方の何れかとする
CvTermCriteria termcrit = cvTermCriteria(CV_TERMCRIT_ITER,50,0.1);
cvNamedWindow("currImg",1);
cvNamedWindow("opImg",1);
cvShowImage("currImg",currImg);
fputs("1: HS\n2: LK\n3: BM\n4: PyrLK\nESC: Quit",stderr);
int key=0;
int method = 4;
while (27 != key){
    cvCopy(currBImg,prevBImg);
    cvCopy(currGImg,prevGImg);
    cvCopy(currRImg,prevRImg);
    currImg = cvQueryFrame(cap);
    cvSplit(currImg,currBImg,currGImg,currRImg,NULL);
    cvCopy(currImg,opImg);
    switch(method){
    case 1:
        ////HSの場合
        //cvCalcOpticalFlowHS(prevBImg,currBImg, 0, vxB, vyB, HS_LAMBDA, termcrit);
        //cvCalcOpticalFlowHS(prevGImg,currGImg, 0, vxG, vyG, HS_LAMBDA, termcrit);
        //cvCalcOpticalFlowHS(prevRImg,currRImg, 0, vxR, vyR, HS_LAMBDA, termcrit);
        //for (y = 0; y< currImg->height; y+=OPT_Y_STEP){
        //for (x = 0; x< currImg->width; x+=OPT_X_STEP){
        //    cvLine(opImg, cvPoint(x,y), cvPoint(x+ cvGetReal2D(vxB,y,x), y+ cvGetReal2D(vyB,y,x)),CV_RGB(0,0,255));
        //    cvLine(opImg, cvPoint(x,y), cvPoint(x+ cvGetReal2D(vxG,y,x), y+ cvGetReal2D(vyG,y,x)),CV_RGB(0,255,0));
        //    cvLine(opImg, cvPoint(x,y), cvPoint(x+ cvGetReal2D(vxR,y,x), y+ cvGetReal2D(vyR,y,x)),CV_RGB(255,0,0));
        //}
        //}
        break;
    case 2:
        ////LKの場合
        //cvCalcOpticalFlowLK(prevBImg,currBImg, cvSize(LK_SIZE,LK_SIZE), vxB, vyB);
        //cvCalcOpticalFlowLK(prevGImg,currGImg, cvSize(LK_SIZE,LK_SIZE), vxG, vyG);
        //cvCalcOpticalFlowLK(prevRImg,currRImg, cvSize(LK_SIZE,LK_SIZE), vxR, vyR);
        //for (y = 0; y< currImg->height; y+=OPT_Y_STEP){
        //for (x = 0; x< currImg->width; x+=OPT_X_STEP){
        //    cvLine(opImg, cvPoint(x,y), cvPoint(x+ cvGetReal2D(vxB,y,x), y+ cvGetReal2D(vyB,y,x)),CV_RGB(0,0,255));
        //    cvLine(opImg, cvPoint(x,y), cvPoint(x+ cvGetReal2D(vxG,y,x), y+ cvGetReal2D(vyG,y,x)),CV_RGB(0,255,0));
        //    cvLine(opImg, cvPoint(x,y), cvPoint(x+ cvGetReal2D(vxR,y,x), y+ cvGetReal2D(vyR,y,x)),CV_RGB(255,0,0));
        //}
        //}
        break;
    case 3:
        ////BMの場合
        //cvCalcOpticalFlowBM(prevBImg,currBImg,cvSize(BM_BLOCK,BM_BLOCK),cvSize(BM_SHIFT,BM_SHIFT),cvSize(BM_MAX,BM_MAX),0,vxB_BM,vyB_BM);
        //cvCalcOpticalFlowBM(prevGImg,currGImg,cvSize(BM_BLOCK,BM_BLOCK),cvSize(BM_SHIFT,BM_SHIFT),cvSize(BM_MAX,BM_MAX),0,vxG_BM,vyG_BM);
        //cvCalcOpticalFlowBM(prevRImg,currRImg,cvSize(BM_BLOCK,BM_BLOCK),cvSize(BM_SHIFT,BM_SHIFT),cvSize(BM_MAX,BM_MAX),0,vxR_BM,vyR_BM);
        //for (y = 0; y< vxB_BM->rows; y++){
        //for (x = 0; x< vxB_BM->cols; x++){
        //    cvLine(opImg, cvPoint(x*BM_BLOCK,y*BM_BLOCK), cvPoint(x*BM_BLOCK+ cvGetReal2D(vxB_BM,y,x), y*BM_BLOCK+ cvGetReal2D(vyB_BM,y,x)),CV_RGB(0,0,255));
        //    cvLine(opImg, cvPoint(x*BM_BLOCK,y*BM_BLOCK), cvPoint(x*BM_BLOCK+ cvGetReal2D(vxG_BM,y,x), y*BM_BLOCK+ cvGetReal2D(vyG_BM,y,x)),CV_RGB(0,255,0));
        //    cvLine(opImg, cvPoint(x*BM_BLOCK,y*BM_BLOCK), cvPoint(x*BM_BLOCK+ cvGetReal2D(vxR_BM,y,x), y*BM_BLOCK+ cvGetReal2D(vyR_BM,y,x)),CV_RGB(255,0,0));
        //}
        //}
        break;
    case 4:
        //PyrLKの場合
        cvGoodFeaturesToTrack (currBImg, eig_imageB, temp_imageB, corners1B, &corner_countB, 0.001, 5, NULL);
        cvGoodFeaturesToTrack (currGImg, eig_imageG, temp_imageG, corners1G, &corner_countG, 0.001, 5, NULL);
        cvGoodFeaturesToTrack (currRImg, eig_imageR, temp_imageR, corners1R, &corner_countR, 0.001, 5, NULL);
        cvCalcOpticalFlowPyrLK(prevBImg, currBImg, prev_pyramidB, curr_pyramidB, corners1B, corners2B, corner_countB, cvSize(LK_SIZE,LK_SIZE), 4, statusB, NULL,termcrit, 0);
        cvCalcOpticalFlowPyrLK(prevGImg, currGImg, prev_pyramidG, curr_pyramidG, corners1G, corners2G, corner_countG, cvSize(LK_SIZE,LK_SIZE), 4, statusG, NULL,termcrit, 0);
        cvCalcOpticalFlowPyrLK(prevRImg, currRImg, prev_pyramidR, curr_pyramidR, corners1R, corners2R, corner_countR, cvSize(LK_SIZE,LK_SIZE), 4, statusR, NULL,termcrit, 0);
        for (x=0; x<corner_countB; x++){
        if (statusB[x]){
            cvLine(opImg,cvPointFrom32f(corners1B[x]),cvPointFrom32f(corners2B[x]),CV_RGB(255,0,0));
        }
        if (statusG[x]){
            cvLine(opImg,cvPointFrom32f(corners1G[x]),cvPointFrom32f(corners2G[x]),CV_RGB(0,255,0));
        }
        if (statusR[x]){
            cvLine(opImg,cvPointFrom32f(corners1R[x]),cvPointFrom32f(corners2R[x]),CV_RGB(0,0,255));
        }
        }
        break;
    default:
        break;
    }
    cvShowImage("currImg",currImg);
    cvShowImage("opImg",opImg);
    key = cvWaitKey(10);
    switch(key){
    case '1':
        method = 1;
        break;
    case '2':
        method = 2;
        break;
    case '3':
        method = 3;
        break;
    case '4':
        method = 4;
        break;
    default:
        break;
    }
}
cvReleaseCapture(&cap);
}
