#include "dynamic.c"
#include <stdlib.h>

void Darray_init(Darray *arr, int init_capacity, int init_value);
void Darray_reserve(Darray *arr, int capacity);
void Darray_push_back_multiple(Darray *arr, int n, Element value);
void Darray_free(Darray *arr);

/* 동적 배열을 초기화 */
void Darray_init(Darray* arr, int init_capacity, Element init_value)
{
	arr->base = 0;
	arr->capacity = 0;
	arr->size = 0;
	
	if(init_capacity > 0)
	{
		Darray_reserver(arr, init_capacity);
		Darray_push_back_multiple(arr, int_capacity, init_value);
	}
}

/* 기존의 동적 배열의 capacity를 확장 */
void Darray_reserve(Darray *arr, int capacity)
{
	arr->base = (Element*)realloc(arr->base, sizeof(Element)*capacity);
	arr->capacity = capacity;
}

/* 동적 배열에 순차적으로 여러 개 추가 */
void Darray_push_back_multiple(Darray *arr, int n, Element value)
{
	for(int i=0; i<n ; i++)
	{
		Darray_push_back(arr, value);
	}
}

/* 동적 배열 초기화 */
void Darray_free(Darray* arr)
{
	if(arr->base)
	{
		free(arr->base);
	}
}


/* 동적 배열을 생성하는 함수 */
Darray* Darray_create(int init_capacity, Element init_value)
{
	Darray *new = (Darray*)malloc(sizeof(Darray));
	Darray_init(new, init_capacity, init_value);
	return new;
}

/* 저장소의 현재 크기(capacity)를 가져오는 함수 */
int Darray_get_capacity(Darray *arr)
{
	return arr->capacity;
}

/* 현재 보관한 요소의 개수(size)를 가져오는 함수 */
int Darray_get_size(Darray *arr)
{
	return arr->size;
}

/* 저장소의 시작 위치를 확인하는 함수 */
Position Darray_begin(Darray *arr)
{
	return arr->base;
}

/* 저장소의 마지막 위치를 확인하는 함수 */
Position Darray_end(Darray *arr)
{
	return (arr->base + arr->size);
}

/* 순서대로 자료를 저장하는 함수 */
void Darray_push_back(Darray *arr, Element data)
{
	if(arr->capacity == arr->size)
	{
		Darray_reserve(arr, arr->capacity*2);	// 배열의 꽉 찼을 경우
	}
	else
	{
		Darray_reserve(arr, 1);
	}
	arr->base[arr->size] = data;
	(arr->size)++;
}

