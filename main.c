#include<reg52.h>

sbit KEY_OUT_1 = P2^3;
sbit KEY_OUT_2 = P2^2;
sbit KEY_OUT_3 = P2^1;
sbit KEY_OUT_4 = P2^0;
sbit KEYSTART = P2^6;
sbit KEYTOWARD = P2^7;
sbit ADDR1 = P1^1;
sbit ADDR2 = P1^2;
sbit ADDR3 = P1^3;
sbit ENLED = P1^4;

unsigned char code LedNum[17] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
								0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E, 0x7F};
unsigned char LedBuff[4] = {0xC0, 0xC0, 0xC0, 0xC0};

unsigned int LedCnt = 0;	//记录T0中断次数 
unsigned int score = 0;		//当前得分
bit start = 0;	//判断是否开始，0为暂停 1为开始/继续

unsigned char NowPicture[8] = {0xFF, 0xBF, 0xFF, 0xFF, 
								0xFD, 0xFD, 0xFD, 0xFF};	//当前刷新行处的P0值
/*
0为亮，与NowPicture相同
unsigned char map[8][8] = {
	{1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,0,1},
	{1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1},
	{1,0,1,1,1,1,1,1},
	{1,0,1,1,1,1,1,1},
	{1,0,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1}
};*/
unsigned char FoodPosition[2] = {1,6};	//默认食物位置 都是行先列后
unsigned int body[3][2] = {
	{4,1},{5,1},{6,1}
	//头 身体 尾
};
char toward[4][2] = {
	{-1,0},{1,0},{0,-1},{0,1}
	//上下左右
};
char NowToward = 0;	//默认向上
//当前按键状态
bit KeyUp = 1;
bit KeyDown = 1;
bit KeyLeft = 1;
bit KeyRight = 1;
bit KeyStart = 1;

void LedScan();	//扫描led
void KeyScan();	//扫描键盘

void main()
{
    EA = 1;       //使能总中断
    ENLED = 0;    //使能U3，选择控制数码管

    TMOD = 0x11;  //设置T0 T1为模式1
    TH0  = 0xFC;  //为T0赋初值0xEC67，定时1ms
    TL0  = 0x67;
    ET0  = 1;     //使能T0中断
    TR0  = 1;     //启动T0
	TR1  = 1;     //启动T1

	while(1)
	{
        if (LedCnt >= 500 && start)  //判断0.5秒定时标志（大于是因为防止暂停时ledcnt还在加）  默认0.5s动一次画面
        {
            //0.5秒定时标志清零
			LedCnt = 0;
			
			//移动蛇
			//熄灭旧尾巴
			NowPicture[body[2][0]] = 1 << body[2][1] | NowPicture[body[2][0]];
			body[2][0] = body[1][0];
			body[2][1] = body[1][1];
			
			body[1][0] = body[0][0];
			body[1][1] = body[0][1];
			
			body[0][0] = body[0][0] + toward[NowToward][0];
			body[0][1] = body[0][1] + toward[NowToward][1];
			if(body[0][0] == -1)	body[0][0] = 7;
			if(body[0][0] == 8)		body[0][0] = 0;
			if(body[0][1] == -1)	body[0][1] = 7;
			if(body[0][1] == 8)		body[0][1] = 0;				
			//点亮新头部
			NowPicture[body[0][0]] = ~(1 << body[0][1]) & NowPicture[body[0][0]];
			
			//吃到食物
			if(body[0][0] == FoodPosition[0] && body[0][1] == FoodPosition[1])
			{
				score++;
				//新食物位置 由T1计时器决定
				FoodPosition[0] = TL1 % 8;
				FoodPosition[1] = TH1 % 8;
			}
			
			//点亮食物
			NowPicture[FoodPosition[0]] = ~(1 << FoodPosition[1]) & NowPicture[FoodPosition[0]];
			
			//以下代码将score按十进制位从低到高依次提取并转为数码管显示字符
            LedBuff[0] = LedNum[score%10];
            LedBuff[1] = LedNum[score/10%10];
            LedBuff[2] = LedNum[score/100%10];
            LedBuff[3] = LedNum[score/1000%10];       
        }	
	}
}

// 定时器0中断服务函数 
void InterruptTimer0() interrupt 1
{
    TH0 = 0xFC;	//重新加载初值
    TL0 = 0x67;
    LedScan();
	KeyScan();
}
void LedScan()
{
	static unsigned char index = 0;		//动态扫描的索引
	LedCnt++;	//中断次数计数值加1
	index++;		//动态扫描的索引加1
	if(index == 12)	index = 0;	//循环
    if (index < 4)  //刷新计分板
    {
		ADDR3 = 1;	//选计分板
		
		//以下代码完成数码管动态扫描刷
		P0 = 0xFF;   //显示消隐
		P1 = (P1 & 0xF8) | index;	//选择位数8 + index
		P0 = LedBuff[index];
		
    }
	else	//刷新led点阵
	{
		ADDR3 = 0;	//选点阵	
		
		//以下代码完成点阵动态扫描刷新
		P0 = 0xFF;   //显示消隐
		P1 = index - 4;	//选择行数
		P0 = NowPicture[index - 4];
	}
}

// 按键扫描函数，需在定时中断中调用，推荐调用间隔1ms 
void KeyScan()
{
	static unsigned char KeyIndex = 0;
	//扫描缓冲区保存一段时间的扫描值,上下左右开始
    static unsigned char KeyUpBuf = 0xFF;
	static unsigned char KeyDonwBuf = 0xFF;
	static unsigned char KeyLeftBuf = 0xFF;
	static unsigned char KeyRightBuf = 0xFF;
	static unsigned char KeyStartBuf = 0xFF;
	
	//开启各个开关，将当前扫描值压入缓冲区
	switch(KeyIndex)
	{
		case 0:
			KEY_OUT_1 = 0;
			KeyUpBuf = (KeyUpBuf << 1) | KEYTOWARD;
			KEY_OUT_1 = 1;	//及时关闭，防止干扰
			KeyIndex++;
			break;
		case 1:
			KEY_OUT_2 = 0;
			KeyLeftBuf = (KeyLeftBuf << 1) | KEYTOWARD;
			KEY_OUT_2 = 1;
			KeyIndex++;
			break;
		case 2:
			KEY_OUT_3 = 0;
			KeyDonwBuf = (KeyDonwBuf << 1) | KEYTOWARD;
			KEY_OUT_3 = 1;
			KeyIndex++;
			break;
		case 3:
			KEY_OUT_4 = 0;
			KeyRightBuf = (KeyRightBuf << 1) | KEYTOWARD;
			KEY_OUT_4 = 1;
			KeyIndex++;
			break;
		default:
			KEY_OUT_4 = 0;
			KeyStartBuf = (KeyStartBuf << 1) | KEYSTART;
			KEY_OUT_4 = 1;
			KeyIndex = 0;
			break;
	}
	
	//连续8次都为0,即40ms，判断为按下
	if(KeyUpBuf == 0x00)
	{
		NowToward = 0;
	}
	else if(KeyDonwBuf == 0x00)
	{
		NowToward = 1;
	}
	else if(KeyLeftBuf == 0x00)
	{
		NowToward = 2;
	}
	else if(KeyRightBuf == 0x00)
	{
		NowToward = 3;
	}
	else if(KeyStartBuf == 0x00)	//防止长按出bug
	{
		KeyStart = 0;
	}
	else if(KeyStartBuf == 0xFF && KeyStart == 0)	//防止长按出bug
	{
		KeyStart = 1;
		start = ~start;
	}
}

