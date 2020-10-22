#include <ARGB.h>

// set target color and trigger a color change
void ARGB::changeColor(uint8_t r, uint8_t g, uint8_t b)
{
  if (ChangeColorState == STDBY)
  {
    targetColor = this->Color(r, g, b);
    if (targetColor != currentColor)
    {
      ChangeColorState = FADE_OUT;
    }
  }
}

// set target color and trigger a color change
void ARGB::changeColor(uint32_t color)
{
  if (ChangeColorState == STDBY)
  {
    targetColor = color;
    Serial.println(String(targetColor) + String(currentColor));
    if (targetColor != currentColor)
    {
      ChangeColorState = FADE_OUT;
    }
  }
}

void ARGB::changeColor(uint32_t color, boolean forceFadeOut)
{
  if (ChangeColorState == STDBY)
  {
    targetColor = color;
    if (targetColor != currentColor || forceFadeOut == true)
    {
      ChangeColorState = FADE_OUT;
    }
  }
}

// state machine to execute a fade-in/out transition color change
void ARGB::changeToColor()
{
  switch (ChangeColorState)
  {
  case STDBY:
    ChangeColorState = STDBY;
    break;
  case FADE_OUT:

    //temporaily saves the brightness to set it later.
    this->oldBrightness = this->currentBrightness;

    //turns the brightness of the stripe to minimal brightness
    setTargetBrightness(MIN_BRIGHTNESS);

    //changes the state
    ChangeColorState = SET_COLOR;
    break;
  case SET_COLOR:
    if (currentBrightness <= targetBrightness)
    {
      // show color at LED strip
      setColor(targetColor);
      setTargetBrightness(this->oldBrightness);
      ChangeColorState = FADE_IN;
    }
    else
    {
      ChangeColorState = SET_COLOR;
    }
    break;
  case FADE_IN:
    if (currentBrightness < targetBrightness)
    {
      ChangeColorState = FADE_IN;
    }
    else
    {
      currentColor = targetColor;
      ChangeColorState = STDBY;
    }
    break;
  default:
    ChangeColorState = STDBY;
  }
}


// change to a new target brightness
void ARGB::gotoBrightness()
{
  int16_t actBrightness = (int16_t)currentBrightness;
  if (timerBrightness.isTimerReady())
  {
    if (targetBrightness > currentBrightness)
    {
      actBrightness += BRIGHTNESS_STEP;
      if (actBrightness > targetBrightness)
        currentBrightness = targetBrightness;
      else
        currentBrightness = actBrightness;
    }
    else if (targetBrightness < currentBrightness)
    {
      actBrightness -= BRIGHTNESS_STEP;
      if (actBrightness < targetBrightness)
        currentBrightness = targetBrightness;
      else
        currentBrightness = (uint8_t)actBrightness;
    }
    // show brighness to LED strip
    setBrightness(currentBrightness);
    timerBrightness.startTimer(timerDuration);
  }
}

// request a storing of light data to EERPOM
bool ARGB::isStoringRequested(void)
{
  return bStoringRequest;
}

// reset a storing request, after light data is stored to EERPOM
void ARGB::resetStoringRequest(void)
{
  bStoringRequest = false;
}

// set brightness and show at LED strip
void ARGB::setBrightness(uint8_t brightness)
{
  // set brighness and show at LED strip
  this->setBrightness(brightness);
  this->show();
}


// set brightness relative to the last value
void ARGB::setBrightnessRelative(int16_t step)
{
  int16_t actBrightness = (int16_t)currentBrightness + step;

  if (actBrightness < MIN_BRIGHTNESS)
  {
    currentBrightness = MIN_BRIGHTNESS;
  }
  else if (actBrightness > MAX_BRIGHTNESS)
  {
    currentBrightness = MAX_BRIGHTNESS;
  }
  else
  {
    currentBrightness = (uint8_t)actBrightness;
  }
  // show brightness to LED strip
  setBrightness(currentBrightness);
  setTargetBrightness(currentBrightness);

  //  start monitoring the change of brightness
  isBrighnessInChange = true;
  timerStoring.startTimer(STORING_DELAY_TIME);
}

// set color and show at LED strip
void ARGB::setColor(uint32_t color)
{
  currentColor = color;
  targetColor = currentColor;

  // set color and show at LED strip
  this->fill(currentColor, 0, this->numLEDs);
  this->show();
}

// set color and show at LED strip
void ARGB::setColor(uint8_t red, uint8_t green, uint8_t blue)
{
  currentColor = this->Color(red, green, blue);
  targetColor = currentColor;

  // set color and show at LED strip
  this->fill(currentColor, 0, this->numLEDs);
  this->show();
}

// set target brighness
void ARGB::setTargetBrightness(uint8_t brightess)
{
  targetBrightness = brightess;
}

uint8_t ARGB::getTargetBrightness(void){
  return targetBrightness;
}

uint8_t ARGB::getCurrentBrightness(void){
  return currentBrightness;
}

// setup light data and send to LED strip
void ARGB::setupARGB(uint8_t brightness, uint32_t color)
{
  targetBrightness = currentBrightness = brightness;
  targetColor = currentColor = color;

  // show brighness and color at LED strip
  setBrightness(brightness);
  this->loop();
}
void ARGB::toggleStripe()
{
  if (isOnb == true)
  {
    isOnb = false;
    StripeOff();
  }
  else
  {
    isOnb = true;
    StripeOn();
  }
}

void ARGB::StripeOff()
{
  if (mosfetPin != 255)
  {
    digitalWrite(mosfetPin, false);
  }
  changeColor(0, 0, 0);
}

void ARGB::StripeOn()
{
  if (mosfetPin != 255)
  {
    digitalWrite(mosfetPin, true);
  }
  changeColor(storage->getSavedColor());
}

// execute a color change and monitor the manual change of brightness
void ARGB::loop()
{
  Serial.println(targetBrightness);
  if (currentBrightness != targetBrightness)
  {
    gotoBrightness();
  }
  else if (ChangeColorState != STDBY)
  {
    changeToColor();
  }
  else if (isBrighnessInChange == true)
  {
    // monitoring manual change of brightness
    if (timerStoring.isTimerReady())
    {
      // last change is longer thnan 5s -> request a storing to EEPROM
      isBrighnessInChange = false;
      bStoringRequest = true;
    }
  }
}