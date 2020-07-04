#ifndef __DYNAMIC_H__
#define __DYNAMIC_H__

typedef char *Element;	    // void*를 재정의
typedef Element *Position;  // 데이터 저장 위치를 Position으로 재정의
typedef struct _Darray {
	Element* base;	// 저장소의 위치 정보
	int capacity;	// 저장소의 현재 크기
	int size;		// 현재 보관한 요소의 개수
} Darray;

Darray* Darray_create(int init_capacity, Element init_value);	// 동적 배열을 생성하는 함수
int Darray_get_capacity(Darray *arr);							// 저장소의 현재 크기(capacity)를 가져오는 함수
int Darray_get_size(Darray *arr);								// 현재 보관한 요소의 개수(size)를 가져오는 함수

Position Darray_begin(Darray *arr);     						// 저장소의 시작 위치를 확인하는 함수
Position Darray_end(Darray *arr);								// 저장소의 마지막 위치를 가져오는 함수

void Darray_push_back(Darray *arr, Element data);				// 순서대로 자료를 저장하는 함수


#endif
