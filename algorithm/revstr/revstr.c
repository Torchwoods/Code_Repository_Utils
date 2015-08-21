#include <stdio.h>
#include <string.h>
char *revstr(char*,int);
int main(int argc,char**argv)
{
	//char *str =  "12345";
	//以这种方式定义，str指向字符串常量区的字符串，而如果后面试图修改常量区域的东西，会出现段错误
	//char str[] = "12345";
	//以这种方式定义是在栈上定义一块空间并将常量区的字符串拷贝一份存放到栈中的数组中，修改栈上的东西是可以的。
	char str[] = "12345";
	printf("%s ",revstr(str,strlen(str)));
	return 0;
}

char *revstr(char* str,int len)
{
	int index;
	if(str != NULL)
	{
		for(index = 0;index<(len/2);index++)
		{
			str[index]  ^= str[len-index-1];
			str[len-index-1] ^= str[index];
			str[index] ^= str[len-index-1];
		}
	}
	return str;
}
