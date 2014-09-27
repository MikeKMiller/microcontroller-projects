package ca.digitalcave.stubby;

import processing.core.PApplet;
import processing.serial.Serial;

public class Stubby {
	//private final PApplet parent;
	private final Protocol protocol;
	
	public Stubby(PApplet parent) {
		//this.parent = parent;
		
		for (String port : Serial.list()){
			if (port.toLowerCase().contains("stubby")){
				final StubbySerial serial = new StubbySerial(parent, port, 38400);
				protocol = new Protocol(serial);
				serial.setProtocol(protocol);
				return;
			}
		}
		
		throw new IllegalArgumentException("No serial port containing text 'stubby' could be found.  Please connect Stubby using a port which name contains text 'stubby', or specify the proper port using the Stubby(PApplet, String) constructor.");
	}
	
	public Stubby(PApplet parent, String port) {
		final StubbySerial serial = new StubbySerial(parent, port, 38400);
		protocol = new Protocol(serial);
		serial.setProtocol(protocol);
	}
	
	public boolean turnOn(){
		return protocol.sendMessage(new Message(Protocol.REQUEST_POWER_ON, new int[0]), 1000, 2);
	}
	public boolean turnOff(){
		return protocol.sendMessage(new Message(Protocol.REQUEST_POWER_OFF, new int[0]), 1000, 2);
	}
	public boolean enableDebug(){
		return protocol.sendMessage(new Message(Protocol.REQUEST_ENABLE_DEBUG, new int[0]), 1000, 2);
	}
	public boolean disableDebug(){
		return protocol.sendMessage(new Message(Protocol.REQUEST_ENABLE_DEBUG, new int[0]), 1000, 2);
	}
	
	/**
	 * Move the specified distance in the indicated direction at the given speed
	 * 
	 * @param linearAngle Angle to move.  0 is straight forward, negative angles are right, and positive angles are left.  Angle in degrees.
	 * @param rotationalAngle Rotational angle to turn.  Measured the same as linear angle.
	 * @param linearVelocity Speed to move; unsigned 8 bit integer value between 255 (fastest) and 0 (slowest).
	 * @param rotationalVelocity Speed to turn; same range as linearVelocity.
	 * @param distance Distance to move (in mm).
	 * @return
	 */
	public boolean move(int linearAngle, int rotationalAngle, int linearVelocity, int rotationalVelocity, int distance){
		if (distance == 0) return true;
		double linearAngleRadians = linearAngle * Math.PI / 180;
		double rotationalAngleRadians = rotationalAngle * Math.PI / 180;
		linearAngleRadians += (float) Math.PI / 2;		//Internally, Stubby uses X axis for steps; here we want to use Y.
		if (distance < 0) linearAngleRadians = linearAngleRadians + Math.PI;	//Support going backwards
		int[] data = new int[6];
		data[0] = Protocol.radianToByte(linearAngleRadians);	
		data[1] = Protocol.radianToByte(rotationalAngleRadians);
		data[2] = linearVelocity & 0xFF;
		data[3] = rotationalVelocity & 0xFF;
		data[4] = (distance >> 8) & 0xFF;
		data[5] = distance & 0xFF;
		if (!protocol.sendMessage(new Message(Protocol.REQUEST_MOVE, data), 1000, 2)){
			return false;
		}
		return protocol.waitForComplete(Protocol.REQUEST_MOVE);
	}
	
	public boolean move(int linearAngle, int distance){
		return move(linearAngle, 0, 255, 0, distance);
	}
	
	/**
	 * Convenience method to walk forward.
	 * @param steps
	 */
	public boolean moveForward(int distance){
		return move(0, distance);
	}

	/**
	 * Convenience method to walk backward.
	 * @param steps
	 */
	public boolean moveBackward(int distance){
		return move(180, distance);
	}
	
	/**
	 * Convenience method to walk right.
	 * @param steps
	 */
	public boolean moveRight(int distance){
		return move(-90, distance);
	}

	/**
	 * Convenience method to walk left.
	 * @param steps
	 */
	public boolean moveLeft(int distance){
		return move(90, distance);
	}
	
	/**
	 * Rotates to the specified angle.  Positive values rotate
	 * clockwise, negative rotate counter clockwise.
	 * @param angle In degrees, with 0 being the direction the robot is currently facing.  Negative angles are to the right (clockwise), positive to the right (counter clockwise)
	 * @param rotationalVelocity Speed to turn, between 0 and 255
	 */
	public boolean turn(int angle, int rotationalVelocity){
		if (angle == 0) return true;
		double angleRadians = angle * Math.PI / -180;
		int[] data = new int[5];
		data[0] = rotationalVelocity & 0xFF;
		Protocol.floatToArr(angleRadians, data, 1);
		if (!protocol.sendMessage(new Message(Protocol.REQUEST_TURN, data), 1000, 2)){
			return false;
		}
		return protocol.waitForComplete(Protocol.REQUEST_TURN);
	}
	/**
	 * Convenience method to turn right
	 * @return
	 */
	public boolean turnRight(){
		return turn(-90, 255);
	}
	/**
	 * Convenience method to turn left
	 * @return
	 */
	public boolean turnLeft(){
		return turn(90, 255);
	}
	/**
	 * Convenience method to turn around 180 degrees (counter clockwise)
	 * @return
	 */
	public boolean turnAround(){
		return turn(180, 255);
	}
	/**
	 * Convenience method to turn around 180 degrees (clockwise)
	 * @return
	 */
	public boolean turnAroundClockwise(){
		return turn(-180, 255);
	}
	
	/**
	 * Reads the distance sensor (value in mm)
	 * @return
	 */
	public long readDistanceSensor(){
		return 0;
	}
	
	/**
	 * Sets the distance servo angle (in radians).  0 is straight forward; negative 
	 * values are looking down, positive values are looking up.
	 * @param angle
	 */
	public void setDistanceAngle(double angle){
		
	}
	
	/**
	 * Reads the specified optical sensor (return value will be a byte between 0 and 255)
	 * @param sensor The sensor index; 0 is furthest left, and increments going right.
	 * @return
	 */
	public int readOpticalSensor(int sensor){
		return 0;
	}
	
	public void dispose(){
		protocol.getSerial().stop();
	}
}