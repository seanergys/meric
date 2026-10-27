program fort_test
  use meric
  use iso_c_binding, only: c_int
  use iso_fortran_env, only: real64
  use omp_lib, only: omp_get_wtime
  implicit none

  integer :: sleep_ms = 1000, run_ms = 1000
  integer :: i, number, status, remaining_ms, chunk_ms
  character(len=64) :: option, argument
  real(real64) :: checksum

  interface
    function c_usleep(microseconds) bind(c, name="usleep") result(result_code)
      import c_int
      integer(c_int), value :: microseconds
      integer(c_int) :: result_code
    end function c_usleep
  end interface

  do i = 1, command_argument_count(), 2
    call get_command_argument(i, option)
    if (trim(option) == "-h") then
      print *, "Usage: fortTest [-s sleep_ms] [-r run_ms]"
      stop
    end if
    call get_command_argument(i + 1, argument)
    read(argument, *) number
    select case (trim(option))
      case ("-s")
        sleep_ms = number
      case ("-r")
        run_ms = number
      case default
        error stop "Unknown option; use -s or -r."
    end select
  end do

  call MERIC_Init()

  call MERIC_MeasureStart("SLEEP"//char(0))
  remaining_ms = sleep_ms
  do while (remaining_ms > 0)
    chunk_ms = min(remaining_ms, 999)
    status = c_usleep(int(chunk_ms, c_int) * 1000_c_int)
    if (status /= 0) error stop "Sleep failed."
    remaining_ms = remaining_ms - chunk_ms
  end do
  call MERIC_MeasureStop("SLEEP"//char(0))

  call MERIC_MeasureStart("RUN"//char(0))
  checksum = 0.0_real64
  !$omp parallel reduction(+:checksum)
  checksum = checksum + do_work(run_ms)
  !$omp end parallel
  call MERIC_MeasureStop("RUN"//char(0))

  call MERIC_Close()
  print *, "Work checksum:", checksum

contains

  function do_work(duration_ms) result(value)
    integer, intent(in) :: duration_ms
    real(real64) :: value, start_time
    integer :: index

    value = 1.0_real64
    start_time = omp_get_wtime()
    do while (omp_get_wtime() - start_time < real(duration_ms, real64) / 1000.0_real64)
      do index = 1, 10000
        value = modulo(value * 1.0000001_real64 + real(index, real64), 1000003.0_real64)
      end do
    end do
  end function do_work

end program fort_test
