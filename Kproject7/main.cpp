//#include <cstdint>            //for int types such as uint32_t
#include "gpiocregisters.hpp" //for GPIOC
#include "gpioaregisters.hpp" //for GPIOA
#include "rccregisters.hpp"   //for RCC
#include "tim2registers.hpp"   //for SPI2
#include "tim5registers.hpp"   //for SPI2
#include "nvicregisters.hpp"  //for NVIC

using namespace std ;

template<typename PortType>
struct AHB1ENR_of
{
};
template<>
struct AHB1ENR_of<GPIOC>
{
  using Enable = RCC::AHB1ENR::GPIOCEN::Enable;
  using Disable = RCC::AHB1ENR::GPIOCEN::Disable;
};
template<>
struct AHB1ENR_of<GPIOA>
{
  using Enable = RCC::AHB1ENR::GPIOAEN::Enable;
  using Disable = RCC::AHB1ENR::GPIOAEN::Disable;
};

template<typename T> 
struct Port
{
  using PortType = T;

  static void Enable()
  {
    AHB1ENR_of<PortType>::Enable::Set();
  }
    static void Disable()
  {
    AHB1ENR_of<PortType>::Disable::Set();
  }
  static void Set(std::uint32_t value)
  {
    T::BSRR::Write(value);
  }
  static void Reset(std::uint32_t value)
  {
    T::BSRR::Write(value << 16U);
  }

  static void Toggle(std::uint32_t value)
  {
    T::ODR::Toggle(value);
  }

  static auto Get()
  {
    return T::IDR::Get();
  }

};

template< typename Port, uint8_t pinNum>
struct Pin
{
  static_assert(pinNum<16U, "There are only 16 pins on port");
  using PortType = typename Port::PortType;
  using PortBase = Port;
  static void Set()
  {
    Port::Set(1U << pinNum);
  }

  static void Reset()
  {
    Port::Reset(1U << pinNum);
  }

  static void Toggle()
  {
    Port::Toggle(1U << pinNum);
  }

  static auto IsSet()
  {
    return ((Port::Get() & (1U << pinNum)) != 0 );
  }
  static void SelectOutputMode()
  {
    volatile auto value = PortType::MODER::Get();
    value &= ~(3U<<(pinNum*2U));
    value |= PortType::MODER::FieldValues::Output::Value<<(pinNum*2U);
    PortType::MODER::Write(value);
  }
  static void SelectInputMode()
  {
    volatile auto value = PortType::MODER::Get();
    value &= ~(3U<<(pinNum*2U));
    value |= PortType::MODER::FieldValues::Input::Value<<(pinNum*2U);
    PortType::MODER::Write(value);
  }
};

template<typename Pin>
struct Button
{  
  static bool WasPressed()
  {
    if(!Pin::IsSet())
    {
      for(int i=0; i< 100000U; ++i)
      {
        asm volatile("");
      }
     if(!Pin::IsSet())
     {
       while(!Pin::IsSet())
       {
         for(int i=0; i< 100000U; ++i)
         {
           asm volatile("");
         }
       }
       return true;
     }
    }
    return false;
  }
  static void Enable()
  {
    Pin::SelectInputMode();
  }
};

template<typename ... PinTypes>
struct Leds
{

  static void Toggle()
  {
    (PinTypes::Toggle(),...);
  }
  static void Reset()
  {
    (PinTypes::Reset(),...);
  }
  static void Enable()
  {
    (PinTypes::PortBase::Enable(),...);
    (PinTypes::SelectOutputMode(),...);
  }
};



constexpr unsigned long long operator""ms(unsigned long long ms)
{
  return ms;
}
constexpr unsigned long long operator""Hz(unsigned long long fr)
{
  return fr;
}
constexpr unsigned long long operator""MHz(unsigned long long fr)
{
  return 1000'000*fr;
}
constexpr unsigned long long operator""kHz(unsigned long long fr)
{
  return 1000*fr;
}

  using PortC = Port<GPIOC>;
  using PortA = Port<GPIOA>;
  using LED1pin = Pin<PortA, 5>;
  using LED2pin = Pin<PortC, 9>;
  using LED3pin = Pin<PortC, 8>;
  using LED4pin = Pin<PortC, 5>;
  using UserLeds = Leds<LED1pin, LED2pin, LED3pin, LED4pin>;
  using UserButton = Button< Pin< PortC, 13> >;
  
    
void MyXmasTim5InterruptHandler()
{
  static uint8_t counterLedNum=1;
  if(TIM5::SR::UIF::UpdatePending::IsSet()&&TIM5::DIER::UIE::Enable::IsSet())
  {
    TIM5::SR::UIF::NoUpdate::Set();    
    switch(counterLedNum)
    {
    case 1:
      LED1pin::Toggle();
      break;
    case 2:
      LED2pin::Toggle();
      break;
    case 3:
      LED3pin::Toggle();
      break;
    case 4:
      LED4pin::Toggle();
      break;
    case 5:
      LED4pin::Toggle();
      break;
    case 6:
      LED3pin::Toggle();
      break;
    case 7:
      LED2pin::Toggle();
      break;
    case 8:
      LED1pin::Toggle();
      break;
    }
    counterLedNum=(counterLedNum<8)?(counterLedNum+1):1;    
  }
}

extern "C"
{
int __low_level_init(void)
{
  RCC::APB1ENR::TIM5EN::Enable::Set();
  TIM5::PSC::Write(16'000U-1U);
  NVIC::ISER1::Write(1<<18U);
  TIM5::DIER::UIE::Enable::Set();
  TIM5::ARR::Write(50U-1U);
  TIM5::CNT::Write(0U);
  return 1;
}
}


int main()
{
  static bool periodDirection =1;
  static uint16_t period = 50ms;
  
  PortC::Enable();
  PortA::Enable();
  UserLeds::Enable();
  UserButton::Enable();
  TIM5::CR1::CEN::Enable::Set();
  for(;;)
  {
    if(UserButton::WasPressed())
    {
      if(period==1000ms)
      {
        periodDirection=0;
      }
      if(period==50ms)
      {
        periodDirection=1;
      }      
      
      if(periodDirection)
      {
        period+=50ms;
      }
      else
      {
        period-=50ms;
      }
      
      TIM5::ARR::Write(period-1U);
      TIM5::CNT::Write(0U);      
    }
  }
  return 0 ;
}

