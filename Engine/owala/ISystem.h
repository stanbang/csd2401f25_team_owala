/*****************************************************************//**
 * \file   ISystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
 * \secondary_author
 *
 * \brief
 *   Abstract base interface for all engine subsystems.
 *   Defines the common lifecycle functions (Initialize, Update, Shutdown)
 *   that CoreEngine uses to manage systems uniformly.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
namespace Framework {
    // -----------------------------------------------------------------
    // ISystem
    //
    // Base interface for all engine subsystems.
    // Any engine system (graphics, input, audio, physics, etc.) should
    // inherit from ISystem and implement the three lifecycle methods:
    //
    //   - Initialize()  : called once before the first Update()
    //   - Update(dt)    : called once per frame with delta time
    //   - Shutdown()    : called once on engine shutdown
    //
    // Example:
    //
    //   class MySystem : public ISystem {
    //   public:
    //       void Initialize() override { /* setup resources */ }
    //       void Update(float dt) override { /* per-frame logic */ }
    //       void Shutdown() override { /* cleanup */ }
    //   };
    //
    struct ISystem {
        /// \brief Virtual destructor for safe polymorphic deletion.
        virtual ~ISystem() = default;

        /**
         * \brief Initialize the system.
         * \details Called once before the first Update(). Allocate resources here.
         */
        virtual void Initialize() = 0;

        /**
         * \brief Per-frame update.
         * \param dt Delta time in seconds since last frame.
         * \details Called once per frame in the CoreEngine::GameLoop().
         */
        virtual void Update(float dt) = 0;

        /**
         * \brief Shutdown the system.
         * \details Called once on engine shutdown. Release resources here.
         */
        virtual void Shutdown() = 0;
    };

} // namespace Framework
