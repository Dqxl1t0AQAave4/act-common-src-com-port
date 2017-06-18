#pragma once

#include <afxwin.h>

class com_port
{

private:

    HANDLE  comm;
    CString comm_name;

public:

    com_port()
        : comm(INVALID_HANDLE_VALUE)
    {
    }

    com_port(const com_port &other) = delete;

    com_port(com_port &&other)
        : comm(other.comm)
        , comm_name(other.comm_name)
    {
        other.comm = INVALID_HANDLE_VALUE;
        other.comm_name = _T("");
    }

    com_port & operator = (const com_port &other) = delete;

    com_port & operator = (com_port &&other)
    {
        if (open())
        {
            close();
        }
        this->comm = other.comm;
        this->comm_name = other.comm_name;
        other.comm = INVALID_HANDLE_VALUE;
        other.comm_name = _T("");
        return *this;
    }

    ~com_port()
    {
        close();
    }

    bool open()
    {
        return (comm != INVALID_HANDLE_VALUE);
    }

    bool operator () ()
    {
        return open();
    }

    bool open(CString name)
    {
        if (open() && (comm_name == name))
        {
            logger::log(_T("port [%s] already opened"), name);
            return true;
        }
        if (open())
        {
            if (!close())
            {
                return false;
            }
            return open0(name);
        }
        else
        {
            return open0(name);
        }
    }

    bool close()
    {
        if (!open())
        {
            return true;
        }
        if (!CloseHandle(comm))
        {
            logger::log(_T("cannot close port [%s] handle"), comm_name);
            return false;
        }
        comm = INVALID_HANDLE_VALUE;
        logger::log(_T("port [%s] closed"), comm_name);
        comm_name = "";
        return true;
    }

public:

    bool read(byte_buffer &dst)
    {
        if (!open())
        {
            logger::log(_T("cannot read from closed port"));
            return false;
        }
        DWORD bytes_read;
        if (!ReadFile(comm, dst.data(), dst.remaining(), &bytes_read, NULL))
        {
            logger::log_system_error(GetLastError());
            logger::log(_T("error while reading the data... closing port [%s]"), comm_name);
            close();
            return false;
        }
        dst.increase_position(bytes_read);
        return true;
    }

    bool write(byte_buffer &src)
    {
        if (!open())
        {
            logger::log(_T("cannot write to closed port"));
            return false;
        }
        DWORD bytes_written;
        if (!WriteFile(comm, src.data(), src.remaining(), &bytes_written, NULL))
        {
            logger::log_system_error(GetLastError());
            logger::log(_T("error while writing the data... closing port [%s]"), comm_name);
            close();
            return false;
        }
        src.increase_position(bytes_written);
        return true;
    }

private:

    bool open0(CString name)
    {
        comm_name = name;
        comm = CreateFile(
            name,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if (comm == INVALID_HANDLE_VALUE)
        {
            DWORD errorMessageID = GetLastError();
            if (errorMessageID == ERROR_FILE_NOT_FOUND)
            {
                logger::log(_T("serial port [%s] does not exist"), name);
            }
            else
            {
                logger::log(_T("error occurred while opening [%s] port"), name);
            }
            logger::log_system_error(errorMessageID);
            return false;
        }

        SetCommMask(comm, EV_RXCHAR);
        SetupComm(comm, 1500, 1500);

        COMMTIMEOUTS CommTimeOuts;
        CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
        CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
        CommTimeOuts.ReadTotalTimeoutConstant = 1000;
        CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
        CommTimeOuts.WriteTotalTimeoutConstant = 1000;

        if (!SetCommTimeouts(comm, &CommTimeOuts))
        {
            logger::log_system_error(GetLastError());
            CloseHandle(comm);
            comm = INVALID_HANDLE_VALUE;
            logger::log(_T("cannot setup port [%s] timeouts"), name);
            return false;
        }

        DCB ComDCM;

        memset(&ComDCM, 0, sizeof(ComDCM));
        ComDCM.DCBlength = sizeof(DCB);
        GetCommState(comm, &ComDCM);
        ComDCM.BaudRate = DWORD(4800);
        ComDCM.ByteSize = 8;
        ComDCM.Parity = ODDPARITY;
        ComDCM.StopBits = ONESTOPBIT;
        ComDCM.fAbortOnError = TRUE;
        ComDCM.fDtrControl = DTR_CONTROL_DISABLE;
        ComDCM.fRtsControl = RTS_CONTROL_DISABLE;
        ComDCM.fBinary = TRUE;
        ComDCM.fParity = TRUE;
        ComDCM.fInX = FALSE;
        ComDCM.fOutX = FALSE;
        ComDCM.XonChar = 0;
        ComDCM.XoffChar = (unsigned char) 0xFF;
        ComDCM.fErrorChar = FALSE;
        ComDCM.fNull = FALSE;
        ComDCM.fOutxCtsFlow = FALSE;
        ComDCM.fOutxDsrFlow = FALSE;
        ComDCM.XonLim = 128;
        ComDCM.XoffLim = 128;

        if (!SetCommState(comm, &ComDCM))
        {
            logger::log_system_error(GetLastError());
            CloseHandle(comm);
            comm = INVALID_HANDLE_VALUE;
            logger::log(_T("cannot setup port [%s] configuration"), name);
            return false;
        }

        logger::log(_T("successfully connected to [%s] port"), name);

        return true;
    }
};