--- hello888547443377
function init()
	reset()
   log("[NOTE] This example demonstrates a four wheeled vehicle. The vehicle appears to slip at the moment and requires some further tuning of internal parameters");
end
function step()
end
function steps()
	log("wheel velocities: " ..
		 string.format("%.2f", robot.joints.base_wheel_bl.encoder) .. ", " ..
		 string.format("%.2f", robot.joints.base_wheel_fl.encoder) .. ", " ..
		 string.format("%.2f", robot.joints.base_wheel_br.encoder) .. ", " ..
		 string.format("%.2f", robot.joints.base_wheel_fr.encoder))
end

function reset()
	robot.joints.base_wheel_bl.set_target(1)
	robot.joints.base_wheel_fl.set_target(1)
	robot.joints.base_wheel_br.set_target(1)
	robot.joints.base_wheel_fr.set_target(1)
end

function destroy()
   -- put your code here
end
