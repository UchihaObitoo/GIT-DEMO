// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
#include "synchconsole.h"

#define MaxFileLength 32
//--------------------------------------------------------------------
// Input: - User space address (int)
// - Limit of buffer (int)
// Output:- Buffer (char*)
// Purpose: Copy buffer from User memory space to System memory space
char *User2System(int virtAddr, int limit)
{
	int i; // index
	int oneChar;
	char *kernelBuf = NULL;
	kernelBuf = new char[limit + 1]; // need for terminal string
	if (kernelBuf == NULL)
		return kernelBuf;
	memset(kernelBuf, 0, limit + 1);
	// printf("\n Filename u2s:");
	for (i = 0; i < limit; i++)
	{
		kernel->machine->ReadMem(virtAddr + i, 1, &oneChar);
		kernelBuf[i] = (char)oneChar;
		// printf("%c",kernelBuf[i]);
		if (oneChar == 0)
			break;
	}
	return kernelBuf;
}

//----------------------------------------------------------
// Input: - User space address (int)
// - Limit of buffer (int)
// - Buffer (char[])
// Output:- Number of bytes copied (int)
// Purpose: Copy buffer from System memory space to User memory space
int System2User(int virtAddr, int len, char *buffer)
{
	if (len < 0)
		return -1;
	if (len == 0)
		return len;
	int i = 0;
	int oneChar = 0;
	do
	{
		oneChar = (int)buffer[i];
		kernel->machine->WriteMem(virtAddr + i, 1, oneChar);
		i++;
	} while (i < len && oneChar != 0);
	return i;
}

void IncreasePC()
{
	kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

	/* set next programm counter for brach execution */
	kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------

void ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

	switch (which)
	{
	case SyscallException:
		DEBUG('a', "\n SC_Create call ...");
		switch (type)
		{
		case SC_Halt:
			DEBUG(dbgSys, "Shutdown, initiated by user program QUoc Ky.\n");

			SysHalt();

			ASSERTNOTREACHED();
			break;

		case SC_Add:
			DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			int result;
			result = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							/* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Add returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)result);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;
			// case SC_Create:
			// {
			// 	DEBUG(dbgSys, "\n SC_Create call ...");
			// 	int virtAddr;
			// 	char *filename;
			// 	DEBUG(dbgSys, "\n Reading virtual address of filename");

			// 	// Lấy tham số tên tập tin từ thanh ghi r4
			// 	virtAddr = kernel->machine->ReadRegister(4);
			// 	DEBUG(dbgSys, "\n Reading filename.");

			// 	// MaxFileLength là = 32
			// 	filename = User2System(virtAddr, MaxFileLength + 1);
			// 	if (filename == NULL)
			// 	{
			// 		printf("\n Not enough memory in system");
			// 		DEBUG(dbgSys, "\n Not enough memory in system");
			// 		kernel->machine->WriteRegister(2, -1); // trả về lỗi cho chương
			// 		// trình người dùng
			// 		delete filename;
			// 		return;
			// 	}
			// 	DEBUG(dbgSys, "\n Finish reading filename.");
			// 	// DEBUG(‘a’,"\n File name : '"<<filename<<"'");
			// 	//  Create file with size = 0
			// 	//  Dùng đối tượng fileSystem của lớp OpenFile để tạo file,
			// 	//  việc tạo file này là sử dụng các thủ tục tạo file của hệ điều
			// 	//  hành Linux, chúng ta không quản ly trực tiếp các block trên
			// 	//  đĩa cứng cấp phát cho file, việc quản ly các block của file
			// 	//  trên ổ đĩa là một đồ án khác
			// 	if (!kernel->fileSystem->Create(filename))
			// 	{
			// 		printf("\n Error create file '%s'", filename);
			// 		kernel->machine->WriteRegister(2, -1);
			// 		delete filename;
			// 		{
			// 			/* set previous programm counter (debugging only)*/
			// 			kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

			// 			/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
			// 			kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

			// 			/* set next programm counter for brach execution */
			// 			kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			// 		}

			// 		return;
			// 	}
			// 	kernel->machine->WriteRegister(2, 0); // trả về cho chương trình
			// 										  // người dùng thành công
			// 	delete filename;
			// 	break;
			// }

		case SC_ReadChar:
		{
			char buffer;

			DEBUG(dbgSys, "\n SC_ReadChar call ...");

			buffer = kernel->synchConsoleIn->GetChar();
			kernel->machine->WriteRegister(2, buffer);
			IncreasePC();
			return;
			ASSERTNOTREACHED();
			break;
		}

		case SC_PrintString:
		{
			DEBUG(dbgSys, "\n SC_PrintString call ...");
			int virtAddr;
			char *buffer;
			virtAddr = kernel->machine->ReadRegister(4); // Lay dia chi cua tham so buffer tu thanh ghi so 4
			buffer = User2System(virtAddr, 255);		 // Copy chuoi tu vung nho User Space sang System Space voi bo dem buffer dai 255 ki tu
			int length = 0;
			while (buffer[length] != 0)
				length++; // Dem do dai that cua chuoi
			for (int i = 0; i < length; i++)
			{
				kernel->synchConsoleOut->PutChar(buffer[i]);
			}
			delete buffer;
			DEBUG(dbgSys, "\n SC_PrintString call ...");
			IncreasePC();
			return;
		}

		case SC_PrintChar:
			int ch;
			ch = kernel->machine->ReadRegister(4);
			kernel->synchConsoleOut->PutChar((char)ch);
			IncreasePC();
			return;

		case SC_PrintInt:
		{
			// Input: mot so integer
			// Output: khong co
			// Chuc nang: In so nguyen len man hinh console
			int number = kernel->machine->ReadRegister(4);
			if (number == 0)
			{
				kernel->synchConsoleOut->PutChar('0'); // In ra man hinh so 0
				IncreasePC();
				return;
			}

			/*Qua trinh chuyen so thanh chuoi de in ra man hinh*/
			bool isNegative = false; // gia su la so duong
			int numberOfNum = 0;	 // Bien de luu so chu so cua number
			int firstNumIndex = 0;

			if (number < 0)
			{
				isNegative = true;
				number = number * -1; // Nham chuyen so am thanh so duong de tinh so chu so
				firstNumIndex = 1;
			}

			int t_number = number; // bien tam cho number
			while (t_number)
			{
				numberOfNum++;
				t_number /= 10;
			}

			// Tao buffer chuoi de in ra man hinh
			char *buffer;
			int MAX_BUFFER = 255;
			buffer = new char[MAX_BUFFER + 1];
			for (int i = firstNumIndex + numberOfNum - 1; i >= firstNumIndex; i--)
			{
				buffer[i] = (char)((number % 10) + 48);
				number /= 10;
			}
			if (isNegative)
			{
				buffer[0] = '-';
				buffer[numberOfNum + 1] = 0;
				for (int i = 0; i < numberOfNum + 1 ; i ++){
					kernel->synchConsoleOut->PutChar(buffer[i]);
				}
				delete buffer;
				IncreasePC();
				return;
			}
			buffer[numberOfNum] = 0;
			for (int i = 0; i < numberOfNum; i ++){
					kernel->synchConsoleOut->PutChar(buffer[i]);
				}
			delete buffer;
			IncreasePC();
			return;
		}
		case SC_ReadString:
		{
			DEBUG(dbgSys, "\n SC_ReadString call ...");
			int addr, lenght;
			char *buffer;

			addr = kernel->machine->ReadRegister(4);
			lenght = kernel->machine->ReadRegister(5);

			char ch;
			buffer = User2System(addr, lenght);
			int i = 0;
			while (true)
			{
				ch = kernel->synchConsoleIn->GetChar();
				if (ch == '\n')
					break;
				buffer[i] = ch;
				i++;
			}
			buffer += '\0';
			System2User(addr, lenght, buffer);

			IncreasePC();
			return;
			break;
		}

		default:
			cerr << "Unexpected system call " << type << "\n";
			break;
		}
		break;

	case NoException:
		DEBUG(dbgSys, "Everything OK.\n");

		return;
		break;

	case PageFaultException:
		DEBUG(dbgSys, "Shutdown, No valid translation found.");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case ReadOnlyException:
		DEBUG(dbgSys, "Shutdown, Write attempted to page marked 'read-only'.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case BusErrorException:
		DEBUG(dbgSys, "Shutdown, Translation resulted in an invalid physical address.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case AddressErrorException:
		DEBUG(dbgSys, "Shutdown, Unaligned reference or one that was beyond the end of the address space.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case OverflowException:
		DEBUG(dbgSys, "Shutdown, Integer overflow in add or sub.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case IllegalInstrException:
		DEBUG(dbgSys, "Shutdown, Unimplemented or reserved instr.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	case NumExceptionTypes:
		DEBUG(dbgSys, "Shutdown, Other exceptions.\n");
		SysHalt();
		ASSERTNOTREACHED();
		break;

	default:
		DEBUG(dbgSys, "Unexpected user mode exception.\n");
		cerr << "Unexpected user mode exception" << (int)which << "\n";
		break;
	}

	ASSERTNOTREACHED();
}
