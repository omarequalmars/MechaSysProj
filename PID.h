int saturated(float ctrlaction, float satlim) { // saturation status
  /* this function returns
    1 if the ctrlaction is upper-limit-saturated
    and -1 if lower-limit-saturated */
  static int flag = 0;
  //  if(ctrlaction>=satlim)
  //    flag=1;
  //
  //  else if(ctrlaction<=-satlim)
  //    flag=-1;
  //  else
  //  flag=0;
  flag = ctrlaction >= satlim ? 1 : ctrlaction <= -satlim ? -1 : 0;
  return flag;
}
float P_ctrlr(float error, float P) {
  static float P_ctrlaction = 0;
  P_ctrlaction = error * P; // calculating ctrl action based on P
  return P_ctrlaction;
}
float I_ctrlr(float sumerror, float I, float T_samp) {
  static float I_ctrlaction = 0;
  I_ctrlaction = T_samp * sumerror * I; // calculating ctrl action based on I
  return I_ctrlaction;
}
float D_ctrlr(float differror, float D, float T_samp) { // calculating ctrl action based on D
  static float D_ctrlaction = 0;
  D_ctrlaction = differror * D / T_samp;
  return D_ctrlaction;
}
float PID_ctrlr_withoverflow(float error, float P, float I, float D, float saturation_limit, float T_samp, int clamping) {
  static float last_error = 0;
  static float last_last_error = 0;
  static float ctrlaction = 0;
  static float last_ctrlaction = 0;
  static int sat = 0;
  static int flag = 0;
  static short clamp_cond = 0;
  //  if (flag) {
  //    if ((ctrlaction ==(last_ctrlaction+0.01) )|| (ctrlaction ==(last_ctrlaction-0.01)))
  //      ctrlaction = 0;
  //  }
  flag = 1;
  // Saturation
  // if(upper limit saturated) => limit the ctrlaction to upper limit
  // if(lower limit saturated) => limit the ctrlaction to lower limit
  if (clamping == 0) {
    ctrlaction = ctrlaction + P * (error - last_error) + I * T_samp * error + D * (error - 2 * last_error + last_last_error) / T_samp;
    last_last_error = last_error;
    last_error = error;
  }
  else if (clamping == 1) {
    clamp_cond = ((sat == 1) && (error > 0)) || ((sat == -1) && (error < 0));
    if (clamp_cond) {
      ctrlaction = ctrlaction + P * (error - last_error) + D * (error - 2 * last_error + last_last_error) / T_samp;
      last_last_error = last_error;
      last_error = error;
    }
    else {
      ctrlaction = ctrlaction + P * (error - last_error) + I * T_samp * error + D * (error - 2 * last_error + last_last_error) / T_samp;
      last_last_error = last_error;
      last_error = error;
    }
  }
  sat = saturated(ctrlaction, saturation_limit);
  if (sat == 1)
    ctrlaction = saturation_limit;
  else if (sat == -1)
    ctrlaction = -saturation_limit;
  return ctrlaction;
}

float PID_ctrlr_withoutoverflow(float error, float P, float I, float D, float saturation_limit, float T_samp, int anti_windup)
{
  static int flag = 0;
  static float integrator_sum = 0;// integrator sum
  static float differror = 0;// error difference
  static float sumerror = 0;// cumulative error sum
  static float last_error = 0;// delayed error
  static short sat = 0;// saturation flag
  static short clamp_cond = 0;// clamping into operation
  static short zero_cross = 0;
  static float ctrlaction = 0;
  // differencing the error
  differror = error - last_error;
//    if (flag > 0) {
//    if (((differror) <= 0.001) || (-(differror/0.005) >= -0.001))
//      error = 0;
//  }

  // delaying the error
  last_error = error;
  // saturation flag
  sat = saturated(ctrlaction, saturation_limit);// check saturated or not
  switch (anti_windup) {
    case 0:// if clamping is deactivated
      sumerror += error;
      ctrlaction = P_ctrlr(error, P) + D_ctrlr(differror, D, T_samp) + I_ctrlr(sumerror, I, T_samp);
      break;
    // if clamping is activated
    default:
      // check if it's clamping time
      zero_cross = (((last_error <= 0) && (error > 0)) || ((last_error >= 0) && (error < 0)));
      clamp_cond = ((sat == 1) && (error > 0)) || ((sat == -1) && (error < 0));
      if (clamp_cond) {// stop integ. in clamping time
        ctrlaction = P_ctrlr(error, P) + D_ctrlr(differror, D, T_samp) + integrator_sum;
      }
      else if (zero_cross) {
        sumerror = 0;
        integrator_sum = 0;
        ctrlaction = P_ctrlr(error, P) + D_ctrlr(differror, D, T_samp) + integrator_sum;
      }
      else {
        sumerror += error;
        integrator_sum = I_ctrlr(sumerror, I, T_samp);
        ctrlaction = P_ctrlr(error, P) + D_ctrlr(differror, D, T_samp) + integrator_sum;
      }
  }
  // Saturation
  // if(upper limit saturated) => limit the ctrlaction to upper limit
  // if(lower limit saturated) => limit the ctrlaction to lower limit
  flag = 1;
  sat = saturated(ctrlaction, saturation_limit);// check saturated or not
  if (sat == 1)
    ctrlaction = saturation_limit;
  else if (sat == -1)
    ctrlaction = -saturation_limit;
  return ctrlaction;
  //ctrlaction = sat == 1 ? saturation_limit : sat == -1 ? -saturation_limit : ctrlaction;
}



float error(float state, float setpoint) { // calculating error
  return (setpoint - state);
}
