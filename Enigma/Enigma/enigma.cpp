#include<stdio.h>
#include<string.h>

const char Rotor3_1[27] = "BDFHJLCPRTXVZNYEIWGAKMUSQO";
const char Rotor3_2[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

const char Rotor4_1[27] = "ESOVPZJAYQUIRHXLNFTGKDCMWB";
const char Rotor4_2[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

const char Rotor5_1[27] = "VZBRGITYUPSDNHLXAWMJQOFECK";
const char Rotor5_2[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

const char RingSetting3 = 'B';
const char RingSetting4 = 'A';
const char RingSetting5 = 'D';

const int Setting3 = RingSetting3 - 'A';
const int Setting4 = RingSetting4 - 'A';
const int Setting5 = RingSetting5 - 'A';

const char Reflect_1[27] = "YRUHQSLDPXNGOKMIEBFZCWVJAT";
const char Reflect_2[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

const char CypherText[48] = "HBLHGWKRPMXKYLKYOHTCPXZATVPPDBZBEXKTZVGEDUYYDJT"; 


int main()
{
	char PlainText[48];

	char result[7];
	int Message3;
	int Message4;
	int Message5;

	int Smessage3;
	int Smessage4;
	int Smessage5;

	int cou;
	int num;
	char CT;
	int delta3;
	int delta4;
	int delta5;

	for (Smessage3 = 0; Smessage3<26; Smessage3++)
	{
		for (Smessage4 = 0; Smessage4<26; Smessage4++)
		{
			for (Smessage5 = 0; Smessage5<26; Smessage5++)
			{
				Message3 = Smessage3;
				Message4 = Smessage4;
				Message5 = Smessage5;

				for (cou = 0; cou <47; cou++)
				{
					Message3 = (Message3 + 1) % 26;//转齿轮
					if (((Message3 - 1) % 26 == 21) || Message4 == 9)
					{
						if (Message4==9)
							Message5 = (Message5 + 1) % 26;

						Message4 = (Message4 + 1) % 26;
					}
						
					CT = CypherText[cou];


					delta3 = Message3 - Setting3;//经过齿轮3
					CT = ((CT - 'A') + delta3 + 26) % 26 + 'A';
					CT = Rotor3_1[CT - 'A'];
					CT = ((CT - 'A') - delta3 + 26) % 26 + 'A';

					delta4 = Message4 - Setting4;//经过齿轮4
					CT = ((CT - 'A') + delta4 + 26) % 26 + 'A';
					CT = Rotor4_1[CT - 'A'];
					CT = ((CT - 'A') - delta4 + 26) % 26 + 'A';

					delta5 = Message5 - Setting5;//经过齿轮5
					CT = ((CT - 'A') + delta5 + 26) % 26 + 'A';
					CT = Rotor5_1[CT - 'A'];
					CT = ((CT - 'A') - delta5 + 26) % 26 + 'A';

					for (num = 0; num < 26; num++)//反射板
					{
						if (Reflect_1[num] == CT)
						{
							CT = Reflect_2[num];
							break;
						}

					}

					delta5 = Message5 - Setting5;//经过齿轮5
					CT = ((CT - 'A') + delta5 + 26) % 26 + 'A';
					for (num = 0; num < 26; num++)
					{
						if (Rotor5_1[num] == CT)
						{
							CT = Rotor5_2[num];
							break;
						}

					}
					CT = ((CT - 'A') - delta5 + 26) % 26 + 'A';

					delta4 = Message4 - Setting4;//经过齿轮4
					CT = ((CT - 'A') + delta4 + 26) % 26 + 'A';
					for (num = 0; num < 26; num++)
					{
						if (Rotor4_1[num] == CT)
						{
							CT = Rotor4_2[num];
							break;
						}

					}
					CT = ((CT - 'A') - delta4 + 26) % 26 + 'A';

					delta3 = Message3 - Setting3;//经过齿轮3
					CT = ((CT - 'A') + delta3 + 26) % 26 + 'A';
					for (num = 0; num < 26; num++)
					{
						if (Rotor3_1[num] == CT)
						{
							CT = Rotor3_2[num];
							break;
						}

					}
					CT = ((CT - 'A') - delta3 + 26) % 26 + 'A';

					PlainText[cou] = CT;

				}

				PlainText[47] = 0;

				for (num = 0; num <42; num++)
				{
					for (int sun = 0; sun < 6; sun++)
					{
						result[sun] = PlainText[num + sun ];
					}
					result[6] = 0;

					if (0 == strcmp(result, "HITLER"))
					{
						printf("MessageKey=%c%c%c\n", (char)(Smessage5 + 'A'), (char)(Smessage4 + 'A'), (char)(Smessage3 + 'A'));
						printf("PlainText=%s\n", PlainText);
						goto SUCCESS;
					}
					
				}





			}
		}
	}
	printf("No result");
	return 0;

   SUCCESS: return 0;


}