#include <stdio.h>
#include <stdlib.h>

#include <memory.h>

#define DAT_FILE "./data/qqwry.dat"
#define OUT_FILE "./data/out.txt"

char *g_temp = NULL;
#define TEMP_SIZE 10*1024*1024
FILE *out = NULL;

void showIp(unsigned int ip)
{
	char buffer[20] = { 0 };
	sprintf_s(buffer, "%u.%u.%u.%u  ", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
	fputs(buffer, out);
}

void getString(FILE *fp)	//讀取字符串并写入文件，以0为结尾标志
{
	int index = 0;
	int k = EOF;
	while (1)
	{
		char c = fgetc(fp);
		if (c == EOF)
			break;
		if (c == 0)
			break;

		g_temp[index] = c;
		index++;
		if (index > TEMP_SIZE)
		{
			//printf("g_temp : %s\n", g_temp);
			printf("exit : %d\n", index);
			fclose(out);
			exit(0);
		}
	}
	fputs(g_temp, out);
	memset(g_temp, 0, index);
}

void getInfo(FILE *fp, int flag)	//获取数据区字符串信息，flag为1时表示已经读了一半字符串
{
	unsigned char mod;
	long int offset = 0;
	long int preOffset = 0;

	mod = fgetc(fp);			//获取模式
	if (mod > 2)				//如果无重定向
	{
		fseek(fp, -1L, SEEK_CUR);	//返回
		getString(fp);			//读取一个字符串
		if (flag == 0)			//如果调用函数时还未读
		{
			getInfo(fp, 1);		//标志置1，递归调用
		}
		return;
	}
	fread(&offset, 3, 1, fp);		//如果有重定向，获取重定向偏移
	if (mod == 1)					//如果模式一
	{
		fseek(fp, offset, SEEK_SET);	//seek到目标地址
		getInfo(fp, flag);				//递归读
		return;
	}
	preOffset = ftell(fp);			//如果模式二，记住当前偏移
	fseek(fp, offset, SEEK_SET);	//寻目标地址
	getInfo(fp, 1);					//读一个字符串
	if (flag == 0)					//如果调用函数时还未读
	{
		fseek(fp, preOffset, SEEK_SET);		//还要回到原来位置
		getInfo(fp, 1);						//再读一次
	}
}

void parseRecord(FILE *fp, long int recordOffset)
{
	unsigned int endIp = 0;

	fseek(fp, recordOffset, SEEK_SET);	//seek到数据区
	fread(&endIp, 4, 1, fp);		//读取结束IP
	showIp(endIp);					//显示结束IP
	getInfo(fp, 0);					//获取信息
	fputs("\n", out);
}

void parseDat(FILE *fp)
{
	long int startIndexOffset = 0;
	long int endIndexOffset = 0;
	long int indexOffset = 0;
	long int recordOffset = 0;
	unsigned int startIp = 0;

	fseek(fp, 0L, SEEK_SET);			//seek文件头
	fread(&startIndexOffset, 4, 1, fp);	//读索引开始偏移量
	fread(&endIndexOffset, 4, 1, fp);	//读索引结束偏移量
	indexOffset = startIndexOffset;
	while (indexOffset <= endIndexOffset)	//循环从根据索引读取数据
	{
		fseek(fp, indexOffset, SEEK_SET);	//seek到索引位置
		fread(&startIp, 4, 1, fp);			//读取索引中的开始IP
		fread(&recordOffset, 3, 1, fp);		//读取对应数据区的偏移量
		indexOffset = ftell(fp);			//记住索引的偏移量
		showIp(startIp);					//显示开始IP
		parseRecord(fp, recordOffset);		//读取数据区
		//printf("indexOffset : %d\n", indexOffset);
	}
}

int main(void)
{
	g_temp = new char[TEMP_SIZE];
	memset(g_temp, 0, TEMP_SIZE);

	FILE *fp = NULL;

	fopen_s(&out, OUT_FILE, "wb");
	fopen_s(&fp, DAT_FILE, "rb");
	if (fp == NULL)
	{
		printf("open file error!\n");
		return 0;
	}
	parseDat(fp);
	fclose(fp);

	delete g_temp;

	return 0;
}
