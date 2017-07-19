#pragma once

#include <afxwin.h>

#include <act-common/logger.h>

namespace com_port_api
{

/**
 *	The structure allows to specify com port options:
 *	baud rate, frame format and port name.
 *	
 *	It also allows to use an existing DCB structure
 *	(set `use_dcb = true` and assign `dcb` an existing structure
 *	or use an appropriate constructor).
 */
struct com_port_options
{
    com_port_options(CString name, DCB dcb)
        : name(name)
        , use_dcb(true)
        , dcb(dcb)
    {
    }

    com_port_options(CString name,
                     size_t baudrate,
                     size_t byte_size,
                     bool   use_parity,
                     size_t parity,
                     size_t stop_bits)
        : name(name)
        , use_dcb(false)
        , baudrate(baudrate)
        , byte_size(byte_size)
        , use_parity(use_parity)
        , parity(parity)
        , stop_bits(stop_bits)
    {
    }

    CString name;

    size_t  baudrate;
    size_t  byte_size;
    bool    use_parity;
    size_t  parity;
    size_t  stop_bits;

    bool    use_dcb;
    DCB     dcb;
};



/**
 *	The class provides a simple interface
 *	over Windows serial port API.
 */
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
    

    /**
     *	Allow only moving constructor to be sure
     *	that only one com_port object holds the connection.
     */
    com_port(const com_port &other) = delete;


    /**
     *	Constructs this object with `other` object
     *	state.
     *	
     *	The `other` object will be in clear state
     *	after this operation, as if it is just created.
     */
    com_port(com_port &&other)
        : comm(other.comm)
        , comm_name(other.comm_name)
    {
        other.comm = INVALID_HANDLE_VALUE;
        other.comm_name = _T("");
    }
    

    /**
     *	Allow only moving semantics to be sure
     *	that only one com_port object holds the connection.
     */
    com_port & operator = (const com_port &other) = delete;
    

    /**
     *	Copies the state of other object into this object.
     *	
     *	If this com_port is open, it will be closed.
     *
     *	If closing fails, the operation just continues.
     *	
     *	The `other` object will be in clear state
     *	after this operation, as if it is just created.
     */
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


    /**
     *	Closes this com_port.
     */
    ~com_port()
    {
        close();
    }


    /**
     *	Checks if this port is open.
     */
    bool open()
    {
        return (comm != INVALID_HANDLE_VALUE);
    }


    /**
     *	Checks if this port is open.
     *	
     *	Equivalent of `open()`.
     */
    bool operator () ()
    {
        return open();
    }


    /**
     *	Opens the port specified by `options.name`
     *	with the given parameters.
     *	
     *	If this port is already opened, it will be closed
     *	and reopened with the specified parameters.
     *	
     *	If closing fails, the operation just continues.
     *	
     *	Returns `true` on success. `false` otherwise.
     */
    bool open(com_port_options options)
    {
        if (open())
        {
            // don't check for result
            // we still cannot do anything with it
            close();

            return open0(options);
        }
        else
        {
            return open0(options);
        }
    }


    /**
     *	Closes this port.
     *	
     *	Returns `true` on success. `false` otherwise.
     */
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


    /**
     *	Reads up to `dst.remaining()` bytes to the `dst` buffer.
     *
     *	If read operation on underlying WinAPI port fails,
     *	this port will be closed.
     *	
     *	Returns `true` on success. `false` otherwise.
     */
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


    /**
     *	Writes up to `dst.remaining()` bytes to the `dst` buffer.
     *
     *	If write operation on underlying WinAPI port fails,
     *	this port will be closed.
     *	
     *	Returns `true` on success. `false` otherwise.
     */
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


    bool open0(com_port_options options)
    {
        comm_name = options.name;
        comm = CreateFile(
            options.name,
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
                logger::log(_T("serial port [%s] does not exist"), options.name);
            }
            else
            {
                logger::log(_T("error occurred while opening [%s] port"), options.name);
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
            logger::log(_T("cannot setup port [%s] timeouts"), options.name);
            return false;
        }

        DCB ComDCM;

        if (options.use_dcb)
        {
            ComDCM = options.dcb;
        }
        else
        {
            memset(&ComDCM, 0, sizeof(ComDCM));
            ComDCM.DCBlength = sizeof(DCB);
            GetCommState(comm, &ComDCM);
            ComDCM.BaudRate = DWORD(options.baudrate);
            ComDCM.ByteSize = options.byte_size;
            ComDCM.Parity = options.parity;
            ComDCM.StopBits = options.stop_bits;
            ComDCM.fAbortOnError = TRUE;
            ComDCM.fDtrControl = DTR_CONTROL_DISABLE;
            ComDCM.fRtsControl = RTS_CONTROL_DISABLE;
            ComDCM.fBinary = TRUE;
            ComDCM.fParity = options.use_parity;
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
        }

        if (!SetCommState(comm, &ComDCM))
        {
            logger::log_system_error(GetLastError());
            CloseHandle(comm);
            comm = INVALID_HANDLE_VALUE;
            logger::log(_T("cannot setup port [%s] configuration"), options.name);
            return false;
        }

        logger::log(_T("successfully connected to [%s] port"), options.name);

        return true;
    }
};

}