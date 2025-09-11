#pragma once
namespace Framework {

	struct ISystem {
		virtual ~ISystem() = default;
		virtual void Initialize() = 0;   // no Core& param
		virtual void Update(float dt) = 0;
		virtual void Shutdown() = 0;
	};

} // namespace Framework
