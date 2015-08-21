#include <stdio.h>

#define SIZE(array) (sizeof(array)/sizeof(array[0]))

void BubbleSort(void *buf,int size);
void ChoiseSort(void *buf,int size);


void main(int argc,char** argv)
{
	int loop = 0;
	int data[] = {5,1,2,3,7,4,0};
//	BubbleSort(data,SIZE(data));
	ChoiseSort(data,SIZE(data));
	for(loop = 0;loop<SIZE(data);loop++)
		printf("%d ",data[loop]);
	printf("\n");
}
/*
	冒泡排序算法
 */
void BubbleSort(void *buf ,int size)
{
	int *data = (int*)buf;
	int outLoop,inLoop;
	for(outLoop = 0; outLoop<size-1;outLoop++)
	{
		for(inLoop=outLoop+1;inLoop<size;inLoop++)
		{
			if(data[outLoop] > data[inLoop]){
				data[outLoop]	^=	data[inLoop];
				data[inLoop] 	^=	data[outLoop];
				data[outLoop] 	^=	data[inLoop];
			}
		}
	}	
}

/*
 *选择排序
 *  
 */
void ChoiseSort(void *buf,int size)
{
	int *data = (int*)buf;
	int outLoop,inLoop;
	int index = 0;
	for(outLoop = 0; outLoop<size-1;outLoop++)
	{
		index = outLoop;
		for(inLoop=outLoop+1;inLoop<size;inLoop++)
		{
			if(data[index] > data[inLoop]){
				index = inLoop;
			 }
			if(outLoop != index){
				data[outLoop]	^=	data[index];
				data[index] 	^=	data[outLoop];
				data[outLoop] 	^=	data[index];
			}
		}
	}	
}
