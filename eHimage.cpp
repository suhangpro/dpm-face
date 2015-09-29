/*
 * eHimage.cpp
 *
 * Hang Su
 * 2012-07 @ eH
 */
#include "eHimage.h"
#include "eHbox.h"

#include <assert.h>
#include <string.h>
#include <iostream>
#include <fstream>

#include <cstring>

#include "rapidxml-1.13/rapidxml.hpp"
#include "rapidxml-1.13/rapidxml_print.hpp"
#include <opencv/cv.h>
#include <opencv/highgui.h>

static inline int round2int(double x) { return (int)(x+0.5);}

image_ptr image_alloc(size_t sizy, size_t sizx, size_t nch){
	image_ptr img = new struct eHimage;
	img->sizy = sizy;
	img->sizx = sizx;
	img->nchannel = nch;
	img->imsize[0] = sizy;
	img->imsize[1] = sizx;
	img->imsize[2] = nch;
	img->data = new double[sizy*sizx*nch];
	for(unsigned i=0;i<nch;i++) {
		img->ch[i] = img->data + i*sizy*sizx;
	}
	img->is_shared = false;
	img->stepy = img->sizy;
	img->stepyx = img->sizy*img->sizx;
	return img;
}

image_ptr image_alloc(size_t sizy, size_t sizx, size_t nch, const double* fillval) {
	image_ptr img = new struct eHimage;
	img->sizy = sizy;
	img->sizx = sizx;
	img->nchannel = nch;
	img->imsize[0] = sizy;
	img->imsize[1] = sizx;
	img->imsize[2] = nch;
	img->data = new double[sizy*sizx*nch];

	for(unsigned i=0;i<nch;i++) {
		img->ch[i] = img->data + i*sizy*sizx;
		for(unsigned xy=0; xy<sizy*sizx; xy++)
			img->ch[i][xy] = fillval[i];
	}
	img->is_shared = false;
	img->stepy = img->sizy;
	img->stepyx = img->sizy*img->sizx;
	return img;
}

void image_delete(image_ptr img){
	if(NULL==img)
		return;
	if(!img->is_shared) {
		if(img->data!=NULL)
			delete[] img->data;
	}
	delete img;
}

void image_zero(image_ptr img, const double* val) {
	if(img==NULL || img->data==NULL || val==NULL) return;
	unsigned ch, y, x;
	for(ch=0; ch<img->nchannel; ch++)
		for(y=0; y<img->sizy; y++)
			for(x=0;x<img->sizx; x++)
				img->ch[ch][x*img->stepy+y]=val[ch];
}

/*
image_ptr image_readColorImage(const char* filename) {
	using namespace cimg_library;
	CImg<unsigned char> img(filename);
	assert(img.spectrum()==3);
	image_ptr im = image_alloc(img.height(),img.width());
	for(int y=0;y<img.height();y++){
		for(int x=0;x<img.width();x++){
			im->ch[0][y+x*img.height()] = (double)*img.data(x,y,0);
			im->ch[1][y+x*img.height()] = (double)*img.data(x,y,1);
			im->ch[2][y+x*img.height()] = (double)*img.data(x,y,2);
		}
	}
	return im;
}

cimg_library::CImgDisplay* image_display(const image_ptr img, const char* winname) {

	using namespace cimg_library;
	CImg<unsigned char> im(img->sizx,img->sizy,1,3);
	for (int y=0; y<im.height();y++) {
		for(int x=0;x<im.width();y++) {
			*im.data(x,y,0) = (unsigned char)img->ch[0][y+x*img->sizy];
			*im.data(x,y,1) = (unsigned char)img->ch[1][y+x*img->sizy];
			*im.data(x,y,2) = (unsigned char)img->ch[2][y+x*img->sizy];
		}
	}
	CImgDisplay* dispwin = new CImgDisplay(im,winname);
	return dispwin;
}
*/
image_ptr image_readJPG(const char* filename) {
	using namespace cv;
	Mat img = imread(filename, 1);
	if(!img.data) return NULL;

	image_ptr im = image_alloc(img.size().height, img.size().width);
	for(unsigned y=0;y<im->sizy;y++) {
		for(unsigned x=0;x<im->sizx;x++) {
			im->ch[0][y+x*im->stepy]=img.at<Vec3b>(y,x).val[2];
			im->ch[1][y+x*im->stepy]=img.at<Vec3b>(y,x).val[1];
			im->ch[2][y+x*im->stepy]=img.at<Vec3b>(y,x).val[0];
		}
	}
	return im;
}

void image_writeJPG(const image_ptr img, const char* filename) {
	using namespace cv;
	Mat M(img->sizy,img->sizx,CV_8UC3);
	for(unsigned int y=0;y<img->sizy;y++) {
		for(unsigned int x=0;x<img->sizx;x++) {
			M.at<Vec3b>(y,x)[2]=img->ch[0][y+x*img->stepy];
			M.at<Vec3b>(y,x)[1]=img->ch[1][y+x*img->stepy];
			M.at<Vec3b>(y,x)[0]=img->ch[2][y+x*img->stepy];
		}
	}
	imwrite(filename,M);
}

void image_display(const image_ptr img, const std::string& winname) {
	using namespace cv;
	Mat M(img->sizy,img->sizx,CV_8UC3);
	for(unsigned int y=0;y<img->sizy;y++) {
		for(unsigned int x=0;x<img->sizx;x++) {
			M.at<Vec3b>(y,x)[2]=img->ch[0][y+x*img->stepy];
			M.at<Vec3b>(y,x)[1]=img->ch[1][y+x*img->stepy];
			M.at<Vec3b>(y,x)[0]=img->ch[2][y+x*img->stepy];
		}
	}
	namedWindow(winname, CV_WINDOW_AUTOSIZE);
	imshow(winname,M);
	waitKey();
}

/* struct used for caching interpolation values */
/* used by image_subsample() */
struct alphainfo {
	int si, di;
	double alpha;
};

/* copy src into dst using pre-computed interpolation values */
/* used by image_subsample() */
void alphacopy(double* src, double*dst, struct alphainfo *ofs, int n) {
	struct alphainfo *end = ofs+n;
	while(ofs != end) {
		dst[ofs->di] += ofs->alpha * src[ofs->si];
		ofs++;
	}
}

/* resize along each column (result is transposed) */
/* used by image_subsample() */
void subsample1dtran(image_ptr src, size_t sheight, 
		image_ptr dst, size_t dheight, size_t width) throw(std::bad_alloc){
	double scale = (double)dheight/(double)sheight;
	double invscale = (double)sheight/(double)dheight;

	/* cache interpolation values since they can be shared 
	 * among different columns*/
	int len = (int)ceil(dheight*invscale) + 2*dheight;
	alphainfo* ofs=new alphainfo[len];
	int k = 0;
	for (unsigned dy=0;dy<dheight;dy++) {
		double fsy1 = dy * invscale;
		double fsy2 = fsy1 + invscale;
		int sy1 = (int)ceil(fsy1);
		int sy2 = (int)floor(fsy2);
		if(sy1-fsy1 > 1e-3) {
			assert(k<len);
			//assert(sy-1 >= 0);
			ofs[k].di = dy*width;
			ofs[k].si = sy1-1;
			ofs[k++].alpha = (sy1-fsy1)*scale;
		}
		for (int sy = sy1;sy<sy2;sy++) {
			assert(k<len);
			assert(sy<(int)sheight);
			ofs[k].di = dy*width;
			ofs[k].si = sy;
			ofs[k++].alpha = scale;
		}
		if(fsy2-sy2 > 1e-3) {
			assert(k<len);
			assert(sy2<(int)sheight);
			ofs[k].di = dy*width;
			ofs[k].si = sy2;
			ofs[k++].alpha = (fsy2-sy2)*scale;
		}
	}
	for (int nch = 0; nch<3; nch++) {
		for (unsigned x = 0; x<width; x++) {
			double *s = src->ch[nch] + x*src->stepy;
			double *d = dst->ch[nch] + x;
			alphacopy(s,d,ofs,k);
		}
	}
	delete[] ofs;
}

/** @brief Fast image subsampling
 *  @note src image is not destroyed
 */
image_ptr image_subsample(const image_ptr img, double scale) {
	if(scale>1 || scale <=0 || img==NULL || img->data==NULL)
		return NULL;
	size_t dst_sizy = (unsigned int)round2int(img->sizy*scale);
	size_t dst_sizx = (unsigned int)round2int(img->sizx*scale);
	double initialval[] = {0, 0, 0};
	image_ptr scaled = image_alloc(dst_sizy, dst_sizx, img->nchannel, initialval);
	image_ptr tmp = image_alloc(img->sizx,dst_sizy, img->nchannel, initialval);
	/* scale in columns, and transposed */
	subsample1dtran(img,img->sizy,tmp,dst_sizy, img->sizx);
	/* scale in (old)rows, and transposed back */
	subsample1dtran(tmp,img->sizx,scaled,dst_sizx,dst_sizy);
	image_delete(tmp);
	return scaled;
}

void resize1dtran(image_ptr src, image_ptr dst) throw(std::bad_alloc){
	double scale = (dst->sizx-1.0) / (src->sizy-1.0);
	double invscale = 1/scale;
	int* pre = new int[dst->sizx];
	double* alpha = new double[dst->sizx];
	pre[0] = 0; alpha[0] = 1.0;
	pre[dst->sizx-1] = src->sizy-2;  alpha[dst->sizx-1] = 0;
	for(unsigned i=1;i<dst->sizx-1;i++) {
		pre[i]=(int)floor(invscale*i);
		alpha[i]=invscale*i-floor(invscale*i);
	}
	unsigned ch, y, x;
	for(ch=0;ch<dst->nchannel;ch++) {
		for(y=0;y<dst->sizy;y++) {
			for(x=0;x<dst->sizx;x++) {
				dst->ch[ch][x*dst->stepy+y] = 
					src->ch[ch][y*src->stepy+pre[x]]*alpha[x] 
					+ src->ch[ch][y*src->stepy+pre[x]+1]*(1-alpha[x]);
			}
		}
	}
	delete[] pre;
	delete[] alpha;
}

image_ptr image_resize(const image_ptr img, double scale) {
	if(scale<=0 || img==NULL || img->data==NULL)
		return NULL;
	size_t dst_sizy = (unsigned)round2int(img->sizy*scale);
	size_t dst_sizx = (unsigned)round2int(img->sizx*scale);
	image_ptr scaled = image_alloc(dst_sizy, dst_sizx, img->nchannel);
	image_ptr tmp = image_alloc(img->sizx, dst_sizy, img->nchannel);
	
	/* scale in colums, and transposed */
	resize1dtran(img,tmp);
	/* scale in (old)rows, and transposed */
	resize1dtran(tmp,scaled);
	image_delete(tmp);
	return scaled;
}

/* reduce along each column (result is transposed) */
/* used by image_reduce() */
void reduce1dtran(image_ptr src, size_t sheight, 
		image_ptr dst, size_t dheight, size_t width) {
	double *s, *d;
	for (int nch = 0; nch<3; nch++) {
		for (unsigned x = 0; x<width; x++) {
			s = src->ch[nch] + x*src->stepy;
			d = dst->ch[nch] + x;

			/* First row */
			*d = s[0]*0.6875 + s[1]*0.2500 + s[2]*0.0625;
			
			/* middle rows */
			for (unsigned y = 1; y<dheight-2;y++) {
				s += 2;
				d += width;
				*d = s[-2]*0.0625 + s[-1]*0.25 + s[0]*0.375 
					+ s[1]*0.25 + s[2]*0.0625;
			}

			/* Last two rows */
			s += 2;
			d += width;
			if (dheight*2 <= sheight) {
				*d = s[-2]*0.0625 + s[-1]*0.25 + s[0]*0.375 
					+ s[1]*0.25 + s[2]*0.0625;
			} else {
				*d = s[1]*0.3125 + s[0]*0.375 + s[-1]*0.25 
					+ s[-2]*0.0625;
			}
			s += 2;
			d += width;
			*d = s[0]*0.6875 + s[-1]*0.25 + s[-2]*0.0625;
		}
	}
}

/*
 * Reduce size to half, using 5-tap binomial filter for anti-aliasing
 * (see Burt & Adelson's Laplacian Pyramid paper for details)
 */
image_ptr image_reduce(const image_ptr img) {
	size_t dst_sizy = (unsigned int)round2int(img->sizy*.5);
	size_t dst_sizx = (unsigned int)round2int(img->sizx*.5);
	image_ptr scaled = image_alloc(dst_sizy, dst_sizx, img->nchannel);
	image_ptr tmp = image_alloc(img->sizx,dst_sizy, img->nchannel);

	/* scale in columns, and transposed */
	reduce1dtran(img,img->sizy,tmp,dst_sizy,img->sizx);
	/* scale in (old)columns, and transposed back */
	reduce1dtran(tmp,img->sizx,scaled,dst_sizx,dst_sizy);

	image_delete(tmp);
	return scaled;
}

image_ptr image_crop(const image_ptr img, fbox_t crop, int* offset, bool shared) {
	image_ptr result;
	fbox_clip(crop, img->imsize);
	ibox_t intcrop = fbox_getibox(&crop);
	if(shared) {
		result = new image_t;
		result->sizx = intcrop.x2-intcrop.x1+1;
		result->sizy = intcrop.y2-intcrop.y1+1;
		result->imsize[0] = result->sizy;
		result->imsize[1] = result->sizx;
		result->stepy = img->stepy;
		result->stepyx = img->stepyx;
		result->is_shared = true;
		result->nchannel = img->nchannel;
		result->imsize[2] = result->nchannel;
		result->data = img->data + (intcrop.x1*img->stepy + intcrop.y1);
		for(unsigned i=0;i<result->nchannel;i++)
			result->ch[i] = result->data + img->stepyx*i;
	} else {
		result = image_alloc(intcrop.y2-intcrop.y1+1,intcrop.x2-intcrop.x1+1,img->nchannel);
		for(unsigned c=0;c<result->nchannel;c++){
			for(unsigned y=0;y<result->sizy;y++) {
				for(unsigned x=0;x<result->sizx;x++) {
					result->data[c*result->stepyx+x*result->stepy+y] = 
						img->data[c*img->stepyx+(x+intcrop.x1)*result->sizy+(y+intcrop.y1)];
				}
			}
		}
	}
	if(offset!=NULL) {
		offset[0] = intcrop.y1;
		offset[1] = intcrop.x1;
	}
	return result;
}

void image_writeDetectionJpg(const image_ptr img, const vector<bbox_t> boxes, const char* filename) {
	using namespace cv;
	Mat M(img->sizy,img->sizx,CV_8UC3);
	for(unsigned y=0;y<img->sizy;y++) {
		for(unsigned x=0;x<img->sizx;x++) {
			M.at<Vec3b>(y,x)[2]=img->ch[0][y+x*img->stepy];
			M.at<Vec3b>(y,x)[1]=img->ch[1][y+x*img->stepy];
			M.at<Vec3b>(y,x)[0]=img->ch[2][y+x*img->stepy];
		}
	}

	for(unsigned i=0;i<boxes.size();i++){
		for(unsigned j=0;j<boxes[i].boxes.size();j++) {
			int x1 = (int)boxes[i].boxes[j].x1;
			int y1 = (int)boxes[i].boxes[j].y1;
			int w = (int)boxes[i].boxes[j].x2-x1;
			int h = (int)boxes[i].boxes[j].y2-y1;
			rectangle(M, Rect(x1, y1, w, h), Scalar(255,0,0));
			circle(M, Point(x1+0.5*w,y1+0.5*h), 2, Scalar(0,0,255), 2);
		}
	}
	imwrite(filename,M);
}

void image_writeDetectionXml(const vector<bbox_t> boxes, const char* filename) {
	using namespace rapidxml;

	/* root */
	xml_document<> doc;
	xml_node<> *root = doc.allocate_node(node_element,"detected_faces");
	char *str_numFace = doc.allocate_string(std::to_string(boxes.size()).c_str());
	xml_attribute<> *attr_numFace = doc.allocate_attribute("total",str_numFace);
	doc.append_node(root);
	root->append_attribute(attr_numFace);

	/* faces */
	xml_node<> *face, *landmarks, *part;
	xml_attribute<> *attr_score, *attr_view, *attr_numLandmark, *attr_id;
	char *str_score, *str_view, *str_numLandmark, *str_id;
	for(unsigned i=0;i<boxes.size();i++) {
		face = doc.allocate_node(node_element,"face");
		root->append_node(face);
		str_id = doc.allocate_string(std::to_string(i+1).c_str());
		attr_id = doc.allocate_attribute("id",str_id);
		face->append_attribute(attr_id);
		str_score = doc.allocate_string(std::to_string(boxes[i].score).c_str());
		attr_score = doc.allocate_attribute("score",str_score);
		face->append_attribute(attr_score);
		str_view = doc.allocate_string(std::to_string(90-15*boxes[i].component).c_str());
		attr_view = doc.allocate_attribute("view",str_view);
		face->append_attribute(attr_view);
		
		/* landmarks */
		landmarks = doc.allocate_node(node_element,"landmarks");
		face->append_node(landmarks);
		str_numLandmark = doc.allocate_string(std::to_string(boxes[i].boxes.size()).c_str());
		attr_numLandmark = doc.allocate_attribute("total",str_numLandmark);
		landmarks->append_attribute(attr_numLandmark);
		xml_attribute<> *attr_x1, *attr_x2, *attr_y1, *attr_y2;
		char *str_x1, *str_x2, *str_y1, *str_y2;
		for (unsigned j=0;j<boxes[i].boxes.size();j++) {
			part = doc.allocate_node(node_element,"part");
			landmarks->append_node(part);
			str_id = doc.allocate_string(std::to_string(j+1).c_str());
			attr_id = doc.allocate_attribute("id",str_id);
			part->append_attribute(attr_id);
			str_x1 = doc.allocate_string(std::to_string(boxes[i].boxes[j].x1).c_str());
			str_x2 = doc.allocate_string(std::to_string(boxes[i].boxes[j].x2).c_str());
			str_y1 = doc.allocate_string(std::to_string(boxes[i].boxes[j].y1).c_str());
			str_y2 = doc.allocate_string(std::to_string(boxes[i].boxes[j].y2).c_str());
			attr_x1	 = doc.allocate_attribute("x1",str_x1);
			attr_x2	 = doc.allocate_attribute("x2",str_x2);
			attr_y1	 = doc.allocate_attribute("y1",str_y1);
			attr_y2	 = doc.allocate_attribute("y2",str_y2);
			part->append_attribute(attr_x1);
			part->append_attribute(attr_x2);
			part->append_attribute(attr_y1);
			part->append_attribute(attr_y2);
		}
	}
	
	std::ofstream xmlout(filename);
	xmlout<<doc;
	xmlout.close();
	
}

void image_showDetection(const image_ptr img, const vector<bbox_t> boxes, const std::string& winname) {
	using namespace cv;
	Mat M(img->sizy,img->sizx,CV_8UC3);
	for(unsigned y=0;y<img->sizy;y++) {
		for(unsigned x=0;x<img->sizx;x++) {
			M.at<Vec3b>(y,x)[2]=img->ch[0][y+x*img->stepy];
			M.at<Vec3b>(y,x)[1]=img->ch[1][y+x*img->stepy];
			M.at<Vec3b>(y,x)[0]=img->ch[2][y+x*img->stepy];
		}
	}

	for(unsigned i=0;i<boxes.size();i++){
		for(unsigned j=0;j<boxes[i].boxes.size();j++) {
			int x1 = (int)boxes[i].boxes[j].x1;
			int y1 = (int)boxes[i].boxes[j].y1;
			int w = (int)boxes[i].boxes[j].x2-x1;
			int h = (int)boxes[i].boxes[j].y2-y1;
			rectangle(M, Rect(x1, y1, w, h), Scalar(255,0,0));
			circle(M, Point(x1+0.5*w,y1+0.5*h), 2, Scalar(0,0,255), 2);
		}
	}
	namedWindow(winname, CV_WINDOW_AUTOSIZE);
	imshow(winname,M);
	waitKey();
}

void image_showFaces(const image_ptr img, const vector<bbox_t> boxes, const std::string& winname) {
	using namespace cv;
	Mat M(img->sizy,img->sizx,CV_8UC3);
	for(unsigned y=0;y<img->sizy;y++) {
		for(unsigned x=0;x<img->sizx;x++) {
			M.at<Vec3b>(y,x)[2]=img->ch[0][y+x*img->stepy];
			M.at<Vec3b>(y,x)[1]=img->ch[1][y+x*img->stepy];
			M.at<Vec3b>(y,x)[0]=img->ch[2][y+x*img->stepy];
		}
	}

	for(unsigned i=0;i<boxes.size();i++){
		int x1 = (int)boxes[i].outer.x1;
		int y1 = (int)boxes[i].outer.y1;
		int w = (int)boxes[i].outer.x2 - x1;
		int h = (int)boxes[i].outer.y2 - y1;
		rectangle(M, Rect(x1,y1,w,h),Scalar(0,0,255),2);
		if(boxes[i].boxes.size()!=68) continue;
		int idxs_nose[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
		int idxs_leye[] = {9, 10, 11, 12, 13, 14};
		int idxs_reye[] = {20, 21, 22, 23, 24, 25};
		int idxs_mout[] = {31, 32, 33, 34, 35, 36, 37, 38, 39, 
			40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50};
		fbox_t nose = fbox_merge(boxes[i].boxes, idxs_nose, 9); 
		fbox_t leye = fbox_merge(boxes[i].boxes, idxs_leye, 6);
		fbox_t reye = fbox_merge(boxes[i].boxes, idxs_reye, 6);
		fbox_t mout = fbox_merge(boxes[i].boxes, idxs_mout, 20);
		rectangle(M, Rect(nose.x1,nose.y1,nose.x2-nose.x1,nose.y2-nose.y1),Scalar(0,255,0),2);
		rectangle(M, Rect(leye.x1,leye.y1,leye.x2-leye.x1,leye.y2-leye.y1),Scalar(255,0,0),2);
		rectangle(M, Rect(reye.x1,reye.y1,reye.x2-reye.x1,reye.y2-reye.y1),Scalar(255,0,0),2);
		rectangle(M, Rect(mout.x1,mout.y1,mout.x2-mout.x1,mout.y2-mout.y1),Scalar(0,255,255),2);
	}
	namedWindow(winname, CV_WINDOW_AUTOSIZE);
	imshow(winname,M);
	waitKey();
}

