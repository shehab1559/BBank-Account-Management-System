#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "STD_TYPES.h"


struct New_Account
{
    u8 Name[100];
    u8 Address[100];
    u32 Age;
    f32 Balance;
    u64 National_ID;
    u64 Guardian_National_ID;
    u32 Account_Password;
    u32 Bank_Account;
    u8 Account_Active;
    struct New_Account *Next;
};

struct New_Account *Head = NULL;

void Cyan(void)
{
    printf("\033[1;36m");
}

void reset()
{
    printf("\033[0m");
}

u64 generate_BankID();
u32 generate_password();
void Create_New_Account(struct New_Account **Head);
void MakeTransaction(unsigned long long Client_Bank_Account_ID_transfer_to, struct New_Account *Sender);
void print_all(struct New_Account *x);
void Change_Account_State(struct New_Account *temp);
void Deposit_in_Account(struct New_Account *temp, float cash_amount);
void Get_Cash(struct New_Account *temp, float cash_amount);
void ChangePassword(struct New_Account *Head);
void FreeMemory(struct New_Account *Head);

int main(void)
{
    u8 ASK_ADMIN_OR_CLIENT = 0;
    u32 ADMIN_ID;
    u32 ADMIN_Password;
    u8 ASK_ADMIN_OPTION = 0;
    u64 Client_Bank_Account_ID;
    u64 Client_Bank_Account_ID_transfer_to;
    f32 cash_amount;
    struct New_Account *temp = Head;
    u8 Cilent_CHoose;
    u8 Open_Existing_Account_choose =0;

    while (1)
    {
        while (1)
        {
            printf("1-Admin ID: 1234\n");
            printf("2-Admin Password: 1234\n\n\n");
            Cyan();
            printf("\t\tWelcome To IMT Bank Account\n\n");
            reset();
            printf("\nEnter 1 for Admin Window\n    OR    \nEnter 2 for Client Window\n");
            scanf("%d", &ASK_ADMIN_OR_CLIENT);
            switch (ASK_ADMIN_OR_CLIENT)
            {
            case 1:
                printf("Enter Admin ID \n");
                scanf("%d", &ADMIN_ID);
                printf("Enter Admin Password\n");
                scanf("%d", &ADMIN_Password);
                if (ADMIN_ID == 1234 && ADMIN_Password == 1234)
                {
                    system("cls");
                    printf("Enter 1 to Create an account\nEnter 2 To Open An Existing Account\nEnter 3 to Exit the System\n");
                    scanf("%d", &ASK_ADMIN_OPTION);
                    while (ASK_ADMIN_OPTION < 0 || ASK_ADMIN_OPTION > 3)
                    {
                        printf("You Enter an Invalid Number Please try again :\n");
                        scanf("%d", &ASK_ADMIN_OPTION);
                    }
                    system("cls");
                    switch (ASK_ADMIN_OPTION)
                    {
                    case 1:
                        system("cls");
                        Create_New_Account(&Head);
                        break;
                    case 2:
                        system("cls");
                        printf("Enter a Client Bank Account ID\n");
                        scanf("%lld", &Client_Bank_Account_ID);


                        // Use a separate pointer for the inner loop
                        struct New_Account *tempInner = Head;

                        while (tempInner != NULL)
                        {
                            if (tempInner->Bank_Account == Client_Bank_Account_ID)
                            {
                                break;
                            }
                            else
                            {
                                tempInner = tempInner->Next;
                            }
                        }

                        if (tempInner != NULL)
                        {
                           printf("Enter 1 to Make Transaction\nEnter 2 to Change Account Password\nEnter 3 to Get Cash\nEnter 4 to Deposit in Account\nEnter any other number to return to main page\n");

                            scanf("%d", &Open_Existing_Account_choose);

                            switch (Open_Existing_Account_choose)
                            {
                            case 1:
                                // Make Transaction
                                printf("Enter the Bank Account ID you want to transfer money to\n");
                                scanf("%lld", &Client_Bank_Account_ID_transfer_to);
                                struct New_Account *tempInner_2 = Head; 

                                while (tempInner_2 != NULL)
                                {
                                    if (tempInner_2->Bank_Account == Client_Bank_Account_ID_transfer_to)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        tempInner_2 = tempInner_2->Next;
                                    }
                                }
                                // tempIntter deh keda waf 2nd client al awel we   tempIntter-2 wa2f 3nd al tany bet3 wale hab3tlo1
                                MakeTransaction(Client_Bank_Account_ID_transfer_to,  tempInner);

                                break;
                            case 2:
                            // Change account state 
                             system("cls");
                             ChangePassword (tempInner );


                            break;
                            case 3:
                            system("cls");
                            printf("Enter amount of Cash u want\n");
                            scanf("%f",&cash_amount);
                            Get_Cash( tempInner,cash_amount);

                            break;
                            case 4:
                            printf("Enter amount of Cash u want\n");
                            scanf("%f",&cash_amount);
                            Deposit_in_Account( tempInner,cash_amount);

                            break;
                            default:
                            system("cls");
                            break;
                            }
                        }
                        else
                        {
                            system("cls");
                            printf("The Bank Account You Want To Open Is Not Found\n");
                        } 
                        break;
                    case 3:
                    system("cls");
                    break;








                    //Code to Exit the Project
                    default:
                    system("cls");
                    printf("Enter a Wrong Number\n");
                        break;
                    }
                }
                else
                {
                    system("cls");
                    printf("Wrong ID or Password\n\n\n");
                    break;
                }
                break;
            case 2:
                system("cls");
            printf("Enter a Client Bank Account ID\n");
                        scanf("%lld", &Client_Bank_Account_ID);

                        // Use a separate pointer for the inner loop
                        struct New_Account *tempInner = Head;

                        while (tempInner != NULL)
                        {
                            if (tempInner->Bank_Account == Client_Bank_Account_ID)
                            {
                                break;
                            }
                            else
                            {
                                tempInner = tempInner->Next;
                            }
                        }
                        if (tempInner != NULL)
                        {
                            printf("Enter 1 to Make Transaction\nEnter 2 to Change Account Password\nEnter 3 to Get Cash\nEnter 4 to Deposit in Account\nEnter any other number to return to main page\n");


                            scanf("%d",&Cilent_CHoose);

                        switch(Cilent_CHoose)
                        {
                            case 1:
                                // Make Transaction
                                printf("Enter the Bank Account ID you want to transfer money to\n");
                                scanf("%lld", &Client_Bank_Account_ID_transfer_to);
                                struct New_Account *tempInner_2 = Head; 

                                while (tempInner_2 != NULL)
                                {
                                    if (tempInner_2->Bank_Account == Client_Bank_Account_ID_transfer_to)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        tempInner_2 = tempInner_2->Next;
                                    }
                                }
                                // tempIntter deh keda waf 2nd client al awel we   tempIntter-2 wa2f 3nd al tany bet3 wale hab3tlo

                                MakeTransaction(Client_Bank_Account_ID_transfer_to,  tempInner);
                                break;
                        case 2:
                        system("cls");
                        ChangePassword(tempInner);
                        /*Change paswaerd code*/
                        break;
                        case 3:
                           system("cls");
                            printf("Enter amount of Cash u want\n");
                            scanf("%f",&cash_amount);
                            Get_Cash( tempInner,cash_amount);
                            break;
                            case 4:
                            system("cls");
                            printf("Enter amount of Cash u want\n");
                            scanf("%f",&cash_amount);
                            Deposit_in_Account( tempInner,cash_amount);
                            break;
                        default:
                        system("cls");
                        // return to main page
                        break;
                        }
                        }
                        else
                        {
                            system("cls");
                            printf("your cilent account bank is wrong\n");
                            break;
                        }
                break;
            default:
                system("cls");
                printf("Invalid input. Please enter 1 for Admin or 2 for Client.\n\n\n");
                break;
            }
        }
    }
    FreeMemory(Head);
    return 0;
}

u64 generate_BankID()
{
    long long random_number = 0;
    srand(time(NULL));
    for (int i = 0; i < 9; i++)
    {
        random_number = random_number * 10 + rand() % 10;
    }
    if (random_number < 999999999)
        random_number *= 10;
    return random_number;
}

u32 generate_password()
{
    int random_number = 0;
    for (int i = 0; i < 4; i++)
    {
        random_number = random_number * 10 + rand() % 10;
    }
    if (random_number < 999)
        random_number *= 10;

    return random_number;
}

void Create_New_Account(struct New_Account **Head)
{
    struct New_Account *First_name = (struct New_Account *)malloc(sizeof(struct New_Account));
    printf("Please Enter your Full Name\n");
    fflush(stdin);
    fgets(First_name->Name, sizeof(First_name->Name), stdin);
    printf("Please Enter your Address\n");
    fflush(stdin);
    fgets(First_name->Address, sizeof(First_name->Address), stdin);
    printf("Enter Your Age\n");
    scanf("%d", &First_name->Age);
    if (First_name->Age >= 21 && First_name->Age > 0)
    {
        printf("Enter your National ID\n");
        scanf("%lld", &First_name->National_ID);
        while (First_name->National_ID >= 99999999999999 || First_name->National_ID <= 9999999999999) //3l4an te5leh yad5al 14 belzabt
        {
            printf("Invalid Guardian ID Please Enter your Guardian National ID:");
            scanf("%lld", &First_name->National_ID);
        }   
    }
    else
    {
        printf("Your age is under 21 please enter your Guardian National ID.\n");
        scanf("%lld", &First_name->Guardian_National_ID);
        while (First_name->Guardian_National_ID >= 99999999999999 || First_name->Guardian_National_ID <= 9999999999999)
        {
            printf("Invalid Guardian ID Please Enter your Guardian National ID:");
            scanf("%lld", &First_name->Guardian_National_ID);
        }
    }
    printf("Please Enter your Balance\n");
    scanf("%f", &First_name->Balance);
    First_name->Account_Password = generate_password();
    First_name->Bank_Account = generate_BankID();
    First_name->Account_Active = 1;
    First_name->Next = NULL;

    if (*Head == NULL)
    {
        *Head = First_name;
    }
    else
    {
        struct New_Account *temp = *Head;
        while (temp->Next != NULL)
        {
            temp = temp->Next;
        }
        temp->Next = First_name;
    }
    print_all(First_name);
}


void MakeTransaction(u64 Client_Bank_Account_ID_transfer_to, struct New_Account * Sender)
{
    printf("\n\nthe sender bank account : ");
    printf("%lld\n", Sender->Bank_Account);
    struct New_Account *temp = Head;
    u8 flag = 0;
    f32 Send_Money;

    while (temp != NULL)
    {
        if (temp->Bank_Account == Client_Bank_Account_ID_transfer_to)
        {
            flag = 1;
            break;
        }
        else
        {
            temp = temp->Next;
        }
    }

    if (flag == 1)
    {
        if (Client_Bank_Account_ID_transfer_to == Sender->Bank_Account)
        {
            printf("You can't send money to yourself\n");
            return;
        }
        else if (temp->Account_Active == 1 && Sender->Account_Active == 1)
        {
            printf("\n\nThe Received Client Bank Account ID: %lld\n", temp ->Bank_Account);
            printf("\nEnter the amount of money that you want to send\n\n");
            scanf("%f", &Send_Money); 

            if (Send_Money > 0 && Sender->Balance >= Send_Money)
            {
                Sender->Balance -= Send_Money;
                temp->Balance += Send_Money;
                system("cls");
                printf("Transaction Successfully\n\n");
                printf("\t\tSender's balance after transaction: %.2f\n\n", Sender->Balance);
                printf("\t\tReceived Client's balance after transaction: %.2f\n\n", temp->Balance);
            }
            else
            {
                printf("Can't send this money\n\n\n");
            }
        }
        else
        {
            printf("One of these accounts is NOT ACTIVE\n\n\n");
            return;
        }
    }
    else
    {
        printf("The Bank Account You Want To Send To Is Not Found\n");
        return;
    }
}


void Change_Account_State ( struct New_Account *temp)
{
    
    u8 client_choose;
    u8 state;
    printf("Enter 1 to Set Account to Active State\nEnter 2 to Set Account to Restricted State\nEnter 3 to  Set Account to Closed\n");
    scanf("%d",&client_choose);
    switch (client_choose)
    {
    case 1:
    temp->Account_Active= 1;
    printf("The account is Active\n");
        break;
    case 2:
    // be4of al account state we yetb3o
    if(temp->Account_Active==1)
    {
        printf("The account is Active\n");
    }
    else
    {
        printf("the account is UnActive\n");
    }
        break;
    case 3 :
    // deh bey3l al ccount un active
    temp->Account_Active=0;
    printf("the account is UnActive\n");
    break;
    default:
    printf("You Enter a Wrong Number\n");
        break;
    }

}

void Get_Cash( struct New_Account *temp ,f32 cash_amount)
{
    
    
    if(temp->Account_Active==1)
    {
        printf("The Account Balance before transiton is %f\n",temp->Balance);
        if(temp->Balance>=cash_amount&&cash_amount>0)
        {
            temp->Balance=(temp->Balance - cash_amount);
            system("cls");
            printf("\t\tComplete Transtion\n\n");
            printf("\t\tYour Cash now is :%f\n",temp->Balance);
        }
        else
        {
            system("cls");
            printf("\n\nYour account Doesnt have this amount of Money\n\n");
        }
    }
        else{
            system("cls");
            printf("your account is unactive\n");
        }
    

}

void Deposit_in_Account( struct New_Account *temp ,f32 cash_amount)
{
    if(temp->Account_Active==1)
    {
        printf("The Account Balance before transiton is %f\n",temp->Balance);
         temp->Balance=temp->Balance +cash_amount;
         system("cls");
        printf("\t\tComplete Transtion\n\n");
        printf("\t\tYour Cash now is :%f\n",temp->Balance);

    }
    else
    {
        system("cls");
        printf("your account is unactive\n");
    }

} 
void FreeMemory(struct New_Account *Head)
{
    struct New_Account *current = Head;
    while (current != NULL)
    {
        struct New_Account *temp = current;
        current = current->Next;
        free(temp);
        free(Head);
    }
}

void ChangePassword(struct New_Account *client) 
{
    u32 new_password;
    printf("Enter your new Password: ");
    fflush(stdin);
    scanf("%d", &new_password);
    client->Account_Password = new_password;
    printf("%d\n", new_password);
    printf("The new password is : %d",client->Account_Password);
    printf("\n\nPassword Changed Successfully\n\n");
    printf("The new password is : %d\n\n",client->Account_Password);
}

void print_all(struct New_Account * x)
{
    system("cls");
    printf("\nName: %s", x->Name);
    printf("Address: %s", x->Address);
    printf("Age: %d \n" , x->Age);
    printf("National ID: %lld \n" , x->National_ID);
    printf("Guardian ID: %lld \n" , x->Guardian_National_ID);
    printf("Bank ID: %lld   \033[1;33mSAVE THIS NUMBER\033[0m\n" , x->Bank_Account);
    printf("Balance: %.2f \n" , x->Balance);
    printf("Password is: %d  \033[1;33mSAVE THIS NUMBER\033[0m\n", x->Account_Password);
    if(x->Account_Active==1)
    {printf("Status:\033[1;32mActive\033[0m\n\n");}
    else 
    {printf("Status:\033[1;31mNot Active\033[0m\n\n");}
}