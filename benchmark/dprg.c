void proc_c()
{
	int i = 10, j = 20;
	int r = i + j;
}

void proc_a()
{
	proc_c();
}

void proc_b()
{
	proc_c();
	int y = 0;
	for(int i =0; i < 10; ++i)
		y++;
}

int main()
{
	proc_a();
	proc_b();
	int x = 0;
	while(x < 10)
	{
		x++;
	}
	return 0;
}

